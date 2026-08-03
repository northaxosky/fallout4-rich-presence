#include "pch.h"

#include "Discord/IpcClient.h"
#include "Discord/Worker.h"
#include "Presence/Activity.h"
#include "Presence/Mailbox.h"

#ifndef NOMINMAX
#	define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#undef ERROR

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <exception>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

namespace
{
	using Clock = std::chrono::steady_clock;

	inline constexpr auto        kHandshakeTimeout = Discord::kIoTimeout;
	inline constexpr auto        kFrameReplyTimeout = Discord::kIoTimeout;
	inline constexpr auto        kStableConnectionPeriod = std::chrono::seconds{ 10 };
	inline constexpr auto        kPollInterval = std::chrono::milliseconds{ 50 };
	inline constexpr auto        kRateWindow = std::chrono::seconds{ 20 };
	inline constexpr std::size_t kMaxSendsPerWindow = 4;
	inline constexpr auto        kInitialBackoff = std::chrono::milliseconds{ 500 };
	inline constexpr auto        kMaximumBackoff = std::chrono::seconds{ 60 };

	class ActivityRateLimiter
	{
	public:
		[[nodiscard]] bool Ready(Clock::time_point a_now)
		{
			Prune(a_now);
			return sends_.size() < kMaxSendsPerWindow;
		}

		void Record(Clock::time_point a_now)
		{
			Prune(a_now);
			sends_.push_back(a_now);
		}

	private:
		void Prune(Clock::time_point a_now)
		{
			while (!sends_.empty() && sends_.front() + kRateWindow < a_now)
			{
				sends_.pop_front();
			}
		}

		std::deque<Clock::time_point> sends_;
	};

	class ReconnectBackoff
	{
	public:
		[[nodiscard]] std::chrono::milliseconds Current() const noexcept { return current_; }

		void Advance() noexcept
		{
			current_ = std::min(current_ * 2, std::chrono::duration_cast<std::chrono::milliseconds>(kMaximumBackoff));
		}

		void Reset() noexcept { current_ = kInitialBackoff; }

	private:
		std::chrono::milliseconds current_{ kInitialBackoff };
	};

	struct PendingCommand
	{
		Presence::ActivityUpdate update;
		Clock::time_point        sentAt;
	};

	class WorkerState
	{
	public:
		WorkerState(Presence::Mailbox& a_mailbox, std::string a_applicationID) :
			mailbox_(a_mailbox),
			applicationID_(std::move(a_applicationID)),
			processID_(::GetCurrentProcessId())
		{}

		void Start()
		{
			std::thread{ [this] { Run(); } }.detach();
		}

	private:
		void Run() noexcept
		{
			try
			{
				RunLoop();
			}
			catch (const std::exception& a_exception)
			{
				try
				{
					REX::ERROR("Discord worker stopped unexpectedly: {}", a_exception.what());
				}
				catch (...)
				{}
			}
			catch (...)
			{
				try
				{
					REX::ERROR("Discord worker stopped unexpectedly");
				}
				catch (...)
				{}
			}
		}

		void RunLoop()
		{
			ReconnectBackoff backoff;
			for (;;)
			{
				try
				{
					DrainMailbox();

					Discord::IpcClient client;
					client.Connect(applicationID_, kHandshakeTimeout);

					pending_.clear();
					rejectedCommand_.reset();
					needsSend_ = hasLatestUpdate_;
					REX::INFO("Discord IPC ready on discord-ipc-{}", client.PipeIndex());
					RunConnected(client, backoff);
				}
				catch (const Discord::IpcError& a_error)
				{
					Recover(backoff, a_error.what());
				}
				catch (const std::exception& a_exception)
				{
					Recover(backoff, a_exception.what());
				}
				catch (...)
				{
					Recover(backoff, "unknown worker failure"sv);
				}
			}
		}

		void RunConnected(Discord::IpcClient& a_client, ReconnectBackoff& a_backoff)
		{
			const auto readyAt = Clock::now();
			auto       connectionProven = false;

			for (;;)
			{
				DrainMailbox();

				Discord::Protocol::Frame frame{};
				while (a_client.TryReceive(frame))
				{
					if (HandleFrame(a_client, frame))
					{
						a_backoff.Reset();
						connectionProven = true;
					}
				}

				CheckPendingTimeouts();
				SendLatest(a_client);

				if (!connectionProven && readyAt + kStableConnectionPeriod < Clock::now())
				{
					a_backoff.Reset();
					connectionProven = true;
				}

				::Sleep(static_cast<DWORD>(kPollInterval.count()));
			}
		}

		void DrainMailbox()
		{
			Presence::ActivityUpdate update;
			while (mailbox_.TryTake(update))
			{
				if (rejectedCommand_ && update != rejectedCommand_->update)
				{
					rejectedCommand_.reset();
				}

				latestUpdate_ = std::move(update);
				hasLatestUpdate_ = true;
				needsSend_ = true;
			}
		}

		[[nodiscard]] bool HandleFrame(Discord::IpcClient& a_client, const Discord::Protocol::Frame& a_frame)
		{
			switch (a_frame.opcode)
			{
				case Discord::Protocol::Opcode::kPing:
					a_client.Send(Discord::Protocol::Opcode::kPong, a_frame.payload);
					return false;
				case Discord::Protocol::Opcode::kClose:
					throw Discord::IpcError{ "Discord closed the IPC connection" };
				case Discord::Protocol::Opcode::kFrame:
					return HandleReply(a_frame.payload);
				default:
					return false;
			}
		}

		[[nodiscard]] bool HandleReply(std::string_view a_payload)
		{
			const auto reply = nlohmann::json::parse(a_payload, nullptr, false);
			if (reply.is_discarded() || !reply.is_object())
			{
				throw Discord::IpcError{ "Discord returned invalid response JSON" };
			}

			const auto nonce = Discord::Protocol::StringField(reply, "nonce");
			if (Discord::Protocol::StringField(reply, "evt") != "ERROR"sv)
			{
				if (nonce.empty() || Discord::Protocol::StringField(reply, "cmd") != "SET_ACTIVITY"sv)
				{
					return false;
				}

				const auto pending = pending_.find(std::string{ nonce });
				if (pending == pending_.end())
				{
					return false;
				}

				pending_.erase(pending);
				return true;
			}

			if (nonce.empty())
			{
				REX::WARN("Discord returned an uncorrelated activity error");
				return false;
			}

			const auto pending = pending_.find(std::string{ nonce });
			if (pending == pending_.end())
			{
				REX::WARN("Discord returned an activity error for unknown nonce '{}'", nonce);
				return false;
			}

			const auto data = reply.find("data");
			const auto error = data != reply.end() && data->is_object() ? &*data : &reply;
			const auto code = error->find("code");
			const auto message = Discord::Protocol::StringField(*error, "message");

			if (hasLatestUpdate_ && latestUpdate_ == pending->second.update)
			{
				rejectedCommand_ = pending->second;
				needsSend_ = false;
			}
			pending_.erase(pending);

			if (code != error->end() && code->is_number_integer())
			{
				REX::ERROR("Discord rejected activity nonce '{}': {} ({})", nonce, message, code->get<std::int64_t>());
			}
			else
			{
				REX::ERROR("Discord rejected activity nonce '{}': {}", nonce, message);
			}
			return false;
		}

		void SendLatest(Discord::IpcClient& a_client)
		{
			if (!needsSend_ || !hasLatestUpdate_ || CurrentUpdateRejected())
			{
				return;
			}

			if (!rateLimiter_.Ready(Clock::now()))
			{
				return;
			}

			const auto  nonce = NextNonce();
			std::string payload;
			try
			{
				payload = Discord::Protocol::MakeSetActivity(latestUpdate_, processID_, nonce);
			}
			catch (const nlohmann::json::exception& a_exception)
			{
				rejectedCommand_ = PendingCommand{
					.update = latestUpdate_,
					.sentAt = Clock::now()
				};
				needsSend_ = false;
				REX::ERROR("Activity JSON serialization failed: {}", a_exception.what());
				return;
			}

			a_client.Send(Discord::Protocol::Opcode::kFrame, payload);
			const auto sentAt = Clock::now();
			rateLimiter_.Record(sentAt);
			pending_.insert_or_assign(nonce, PendingCommand{
												 .update = latestUpdate_,
												 .sentAt = sentAt });
			needsSend_ = false;
			REX::DEBUG("Sent Discord activity nonce '{}'", nonce);
		}

		void CheckPendingTimeouts() const
		{
			const auto now = Clock::now();
			for (const auto& [nonce, command] : pending_)
			{
				if (command.sentAt + kFrameReplyTimeout < now)
				{
					throw Discord::IpcError{ std::format("Discord did not acknowledge activity nonce '{}'", nonce) };
				}
			}
		}

		void Recover(ReconnectBackoff& a_backoff, std::string_view a_reason) noexcept
		{
			try
			{
				PrepareReconnect();
			}
			catch (...)
			{
				pending_.clear();
				needsSend_ = hasLatestUpdate_;
			}

			const auto delay = a_backoff.Current();
			try
			{
				REX::WARN("Discord IPC unavailable: {}; retrying in {} ms", a_reason, delay.count());
			}
			catch (...)
			{}

			::Sleep(static_cast<DWORD>(delay.count()));
			a_backoff.Advance();
		}

		void PrepareReconnect()
		{
			pending_.clear();
			needsSend_ = hasLatestUpdate_ && !CurrentUpdateRejected();
		}

		[[nodiscard]] bool CurrentUpdateRejected() const
		{
			return hasLatestUpdate_ && rejectedCommand_ && latestUpdate_ == rejectedCommand_->update;
		}

		[[nodiscard]] std::string NextNonce()
		{
			return std::format("{}-{}", processID_, ++nextNonce_);
		}

		Presence::Mailbox&                              mailbox_;
		std::string                                     applicationID_;
		DWORD                                           processID_;
		ActivityRateLimiter                             rateLimiter_;
		Presence::ActivityUpdate                        latestUpdate_;
		std::optional<PendingCommand>                   rejectedCommand_;
		std::unordered_map<std::string, PendingCommand> pending_;
		std::uint64_t                                   nextNonce_{ 0 };
		bool                                            hasLatestUpdate_{ false };
		bool                                            needsSend_{ false };
	};

	// F4SE has no safe shutdown callback, so process exit reclaims the leaked worker.
	std::atomic<WorkerState*> g_worker{ nullptr };
}

namespace Discord
{
	bool Worker::Start(Presence::Mailbox& a_mailbox, std::string a_applicationID) noexcept
	{
		if (g_worker.load(std::memory_order_acquire))
		{
			return true;
		}

		try
		{
			auto worker = std::make_unique<WorkerState>(a_mailbox, std::move(a_applicationID));
			worker->Start();
			g_worker.store(worker.release(), std::memory_order_release);
			return true;
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::ERROR("Discord worker initialization failed: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::ERROR("Discord worker initialization failed");
			}
			catch (...)
			{}
		}
		return false;
	}
}
