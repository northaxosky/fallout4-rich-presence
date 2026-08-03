#include "pch.h"

#include "Discord/IpcClient.h"

#ifndef NOMINMAX
#	define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
	using Clock = std::chrono::steady_clock;
	using Deadline = std::optional<Clock::time_point>;

	[[nodiscard]] std::string ErrorMessage(std::string_view a_operation, DWORD a_error)
	{
		return std::format("{} failed (Win32 error {})", a_operation, a_error);
	}

	[[nodiscard]] DWORD WaitMilliseconds(Deadline a_deadline)
	{
		if (!a_deadline)
		{
			return INFINITE;
		}

		const auto now = Clock::now();
		if (now >= *a_deadline)
		{
			return 0;
		}

		const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(*a_deadline - now).count();
		return static_cast<DWORD>(std::min<std::int64_t>(remaining, INFINITE - 1));
	}

	void CancelPending(HANDLE a_pipe, OVERLAPPED& a_overlapped) noexcept
	{
		::CancelIoEx(a_pipe, &a_overlapped);
		DWORD ignored = 0;
		::GetOverlappedResult(a_pipe, &a_overlapped, &ignored, TRUE);
	}

	template <class Operation>
	[[nodiscard]] DWORD TransferChunk(HANDLE a_pipe, HANDLE a_ioEvent, Deadline a_deadline, Operation&& a_operation)
	{
		::ResetEvent(a_ioEvent);
		OVERLAPPED overlapped{};
		overlapped.hEvent = a_ioEvent;

		DWORD transferred = 0;
		if (std::forward<Operation>(a_operation)(overlapped, transferred))
		{
			return transferred;
		}

		const auto error = ::GetLastError();
		if (error != ERROR_IO_PENDING)
		{
			throw Discord::IpcError{ ErrorMessage("named-pipe I/O", error) };
		}

		const auto wait = ::WaitForSingleObject(a_ioEvent, WaitMilliseconds(a_deadline));
		if (wait == WAIT_TIMEOUT)
		{
			CancelPending(a_pipe, overlapped);
			throw Discord::IpcError{ "named-pipe I/O timed out" };
		}
		if (wait != WAIT_OBJECT_0)
		{
			const auto waitError = ::GetLastError();
			CancelPending(a_pipe, overlapped);
			throw Discord::IpcError{ ErrorMessage("WaitForSingleObject", waitError) };
		}

		if (!::GetOverlappedResult(a_pipe, &overlapped, &transferred, FALSE))
		{
			throw Discord::IpcError{ ErrorMessage("GetOverlappedResult", ::GetLastError()) };
		}

		return transferred;
	}

	[[nodiscard]] std::string HandshakeError(const nlohmann::json& a_message)
	{
		const auto data = a_message.find("data");
		const auto source = data != a_message.end() && data->is_object() ? &*data : &a_message;
		const auto code = source->find("code");
		const auto message = Discord::Protocol::StringField(*source, "message");
		if (code != source->end() && code->is_number_integer())
		{
			return std::format("Discord rejected the handshake: {} ({})", message, code->get<std::int64_t>());
		}
		return std::format("Discord rejected the handshake: {}", message);
	}

	[[nodiscard]] std::string HandshakeCloseError(std::string_view a_payload)
	{
		const auto message = nlohmann::json::parse(a_payload, nullptr, false);
		return message.is_object() ? HandshakeError(message) : "Discord closed IPC during the handshake";
	}
}

namespace Discord
{
	IpcClient::IpcClient()
	{
		ioEvent_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!ioEvent_)
		{
			throw IpcError{ ErrorMessage("CreateEventW", ::GetLastError()) };
		}
	}

	IpcClient::~IpcClient()
	{
		Close();
		if (ioEvent_)
		{
			::CloseHandle(ioEvent_);
			ioEvent_ = nullptr;
		}
	}

	void IpcClient::Connect(std::string_view a_applicationID, std::chrono::milliseconds a_handshakeTimeout)
	{
		Close();

		bool        foundEndpoint = false;
		std::string lastFailure;
		for (int index = 0; index < 10; ++index)
		{
			if (!Open(index))
			{
				continue;
			}

			foundEndpoint = true;
			try
			{
				Handshake(a_applicationID, a_handshakeTimeout);
				return;
			}
			catch (const IpcError& a_error)
			{
				lastFailure = std::format("discord-ipc-{}: {}", index, a_error.what());
				Close();
			}
		}

		if (foundEndpoint)
		{
			throw IpcError{ std::format("all Discord IPC endpoints failed; last failure: {}", lastFailure) };
		}
		throw IpcError{ "no Discord IPC endpoint found" };
	}

	void IpcClient::Close() noexcept
	{
		if (IsConnected())
		{
			::CancelIoEx(pipe_, nullptr);
			::CloseHandle(pipe_);
			pipe_ = INVALID_HANDLE_VALUE;
		}
		pipeIndex_ = -1;
	}

	void IpcClient::Handshake(std::string_view a_applicationID, std::chrono::milliseconds a_timeout)
	{
		const auto  deadline = Clock::now() + a_timeout;
		std::string handshake;
		try
		{
			handshake = Protocol::MakeHandshake(a_applicationID);
		}
		catch (const nlohmann::json::exception& a_exception)
		{
			throw IpcError{ std::format("Discord handshake serialization failed: {}", a_exception.what()) };
		}
		Send(Protocol::Opcode::kHandshake, handshake, deadline);

		while (true)
		{
			auto frame = Receive(deadline);
			switch (frame.opcode)
			{
				case Protocol::Opcode::kPing:
					Send(Protocol::Opcode::kPong, frame.payload, deadline);
					break;
				case Protocol::Opcode::kClose:
					throw IpcError{ HandshakeCloseError(frame.payload) };
				case Protocol::Opcode::kFrame:
				{
					const auto message = nlohmann::json::parse(frame.payload, nullptr, false);
					if (message.is_discarded() || !message.is_object())
					{
						throw IpcError{ "Discord returned invalid handshake JSON" };
					}

					const auto event = Protocol::StringField(message, "evt");
					if (Protocol::StringField(message, "cmd") == "DISPATCH"sv && event == "READY"sv)
					{
						return;
					}
					if (event == "ERROR"sv)
					{
						throw IpcError{ HandshakeError(message) };
					}
					break;
				}
				default:
					break;
			}
		}
	}

	void IpcClient::Send(Protocol::Opcode a_opcode, std::string_view a_payload)
	{
		Send(a_opcode, a_payload, Clock::now() + kIoTimeout);
	}

	bool IpcClient::TryReceive(Protocol::Frame& a_frame)
	{
		if (!IsConnected())
		{
			throw IpcError{ "Discord IPC is not connected" };
		}

		DWORD available = 0;
		if (!::PeekNamedPipe(pipe_, nullptr, 0, nullptr, &available, nullptr))
		{
			throw IpcError{ ErrorMessage("PeekNamedPipe", ::GetLastError()) };
		}
		if (available == 0)
		{
			return false;
		}

		a_frame = Receive(Clock::now() + kIoTimeout);
		return true;
	}

	int IpcClient::PipeIndex() const noexcept
	{
		return pipeIndex_;
	}

	void IpcClient::ReadExact(std::span<std::uint8_t> a_output, Deadline a_deadline)
	{
		auto remaining = a_output;
		while (!remaining.empty())
		{
			const auto transferred = TransferChunk(pipe_, ioEvent_, a_deadline, [this, &remaining](OVERLAPPED& a_overlapped, DWORD& a_transferred) {
				return ::ReadFile(
					pipe_,
					remaining.data(),
					static_cast<DWORD>(std::min<std::size_t>(remaining.size(), std::numeric_limits<DWORD>::max())),
					&a_transferred,
					&a_overlapped);
			});
			if (transferred == 0)
			{
				throw IpcError{ "Discord IPC closed while reading" };
			}
			remaining = remaining.subspan(transferred);
		}
	}

	void IpcClient::WriteExact(std::span<const std::uint8_t> a_input, Deadline a_deadline)
	{
		auto remaining = a_input;
		while (!remaining.empty())
		{
			const auto transferred = TransferChunk(pipe_, ioEvent_, a_deadline, [this, &remaining](OVERLAPPED& a_overlapped, DWORD& a_transferred) {
				return ::WriteFile(
					pipe_,
					remaining.data(),
					static_cast<DWORD>(std::min<std::size_t>(remaining.size(), std::numeric_limits<DWORD>::max())),
					&a_transferred,
					&a_overlapped);
			});
			if (transferred == 0)
			{
				throw IpcError{ "Discord IPC closed while writing" };
			}
			remaining = remaining.subspan(transferred);
		}
	}

	void IpcClient::Send(Protocol::Opcode a_opcode, std::string_view a_payload, Deadline a_deadline)
	{
		if (!IsConnected())
		{
			throw IpcError{ "Discord IPC is not connected" };
		}
		if (a_payload.size() > Protocol::kMaxPayloadSize)
		{
			throw IpcError{ "Discord IPC payload exceeds the frame limit" };
		}

		const auto header = Protocol::EncodeHeader(a_opcode, static_cast<std::uint32_t>(a_payload.size()));
		// Discord requires one initial write for the header and payload.
		std::vector<std::uint8_t> frame;
		frame.reserve(header.size() + a_payload.size());
		frame.insert(frame.end(), header.begin(), header.end());
		frame.insert(frame.end(), a_payload.begin(), a_payload.end());
		WriteExact(frame, a_deadline);
	}

	Protocol::Frame IpcClient::Receive(Deadline a_deadline)
	{
		std::array<std::uint8_t, Protocol::kHeaderSize> encodedHeader{};
		ReadExact(encodedHeader, a_deadline);

		const auto header = Protocol::DecodeHeader(encodedHeader);
		if (header.payloadSize > Protocol::kMaxPayloadSize)
		{
			throw IpcError{ std::format("Discord IPC payload length {} exceeds the frame limit", header.payloadSize) };
		}

		Protocol::Frame frame{
			.opcode = header.opcode,
			.payload = std::string(header.payloadSize, '\0')
		};
		if (!frame.payload.empty())
		{
			ReadExact(std::span{ reinterpret_cast<std::uint8_t*>(frame.payload.data()), frame.payload.size() }, a_deadline);
		}
		return frame;
	}

	bool IpcClient::IsConnected() const noexcept
	{
		return pipe_ != INVALID_HANDLE_VALUE;
	}

	bool IpcClient::Open(int a_index)
	{
		auto pipeName = std::wstring{ LR"(\\?\pipe\discord-ipc-0)" };
		pipeName.back() = static_cast<wchar_t>(L'0' + a_index);

		const auto pipe = ::CreateFileW(
			pipeName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			nullptr);
		if (pipe == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		pipe_ = pipe;
		pipeIndex_ = a_index;
		return true;
	}
}
