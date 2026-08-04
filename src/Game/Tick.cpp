#include "pch.h"

#include "Config.h"
#include "Game/LocationResolver.h"
#include "Game/PlayerState.h"
#include "Game/QuestResolver.h"
#include "Game/Snapshot.h"
#include "Game/Tick.h"
#include "Presence/Mailbox.h"
#include "Presence/StateMachine.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	using Clock = std::chrono::steady_clock;
	using ChainedCall = void (*)(void*, RE::NiAVObject*);

	inline constexpr REL::VariantID Main_OnIdle{ 633524, 2228917 };
	inline constexpr std::ptrdiff_t kOGHookOffset = 0x76;
	inline constexpr std::ptrdiff_t kNGHookOffset = 0x158;
	inline constexpr std::size_t    kAnchorOffset = 0x11;
	inline constexpr std::size_t    kWindowPrefix = 0xE;
	inline constexpr std::size_t    kWindowSize = 0x34;
	inline constexpr auto           kExceptionLogInterval = std::chrono::seconds{ 30 };

	inline constexpr std::array<std::uint8_t, 3>  kMovRdx{ 0x48, 0x8B, 0x15 };
	inline constexpr std::array<std::uint8_t, 3>  kMovRcx{ 0x48, 0x8B, 0x0D };
	inline constexpr std::array<std::uint8_t, 7>  kAnchor{ 0xB9, 0x09, 0x00, 0x00, 0x00, 0xFF, 0x15 };
	inline constexpr std::array<std::uint8_t, 10> kAfterAnchor{ 0xBB, 0x00, 0x80, 0x00, 0x00, 0x66, 0x85, 0xC3, 0x74, 0x14 };

	[[nodiscard]] std::int64_t UnixTimestamp() noexcept;

	struct TickState
	{
		Clock::time_point lastSample{};
		Clock::time_point nextExceptionLog{};
		std::uint64_t     tickCount{ 0 };
		std::uint64_t     suppressedExceptions{ 0 };
		bool              hasLoggedException{ false };
	};

	ChainedCall              g_chainedCall{ nullptr };
	TickState                g_tickState{};
	Game::QuestResolver      g_questResolver{};
	Game::LocationResolver   g_locationResolver{};
	Presence::Mailbox        g_mailbox{};
	Presence::StateMachine   g_stateMachine{};
	Presence::ActivityUpdate g_lastPublished{};
	std::int64_t             g_startTimestamp{ UnixTimestamp() };
	bool                     g_hasPublished{ false };

	template <std::size_t N>
	[[nodiscard]] bool Matches(const std::uint8_t* a_bytes, const std::array<std::uint8_t, N>& a_expected)
	{
		return std::equal(a_expected.begin(), a_expected.end(), a_bytes);
	}

	[[nodiscard]] bool ValidateSite(std::uintptr_t a_site) noexcept
	{
		const auto bytes = reinterpret_cast<const std::uint8_t*>(a_site);
		return Matches(bytes - 0xE, kMovRdx) &&
		       Matches(bytes - 0x7, kMovRcx) &&
		       bytes[0] == 0xE8 &&
		       Matches(bytes + 0x5, kMovRcx) &&
		       bytes[0xC] == 0xE8 &&
		       Matches(bytes + kAnchorOffset, kAnchor) &&
		       Matches(bytes + 0x1C, kAfterAnchor);
	}

	[[nodiscard]] bool HasAddressLibrary(const REL::Version& a_runtime)
	{
		return std::filesystem::exists(std::format("Data/F4SE/Plugins/version-{}.bin", a_runtime.string("-")));
	}

	[[nodiscard]] std::string FormatBytes(std::uintptr_t a_site)
	{
		const auto  bytes = reinterpret_cast<const std::uint8_t*>(a_site - kWindowPrefix);
		std::string result;
		result.reserve(kWindowSize * 3);

		for (std::size_t i = 0; i < kWindowSize; ++i)
		{
			if (i != 0)
			{
				result.push_back(' ');
			}
			std::format_to(std::back_inserter(result), "{:02X}", static_cast<unsigned int>(bytes[i]));
		}

		return result;
	}

	[[nodiscard]] std::int64_t UnixTimestamp() noexcept
	{
		return std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch())
		    .count();
	}

	void RunTick()
	{
		const auto now = Clock::now();
		const auto elapsed = now - g_tickState.lastSample;
		if (elapsed < Config::GetSamplingInterval())
		{
			return;
		}

		g_tickState.lastSample = now;
		++g_tickState.tickCount;

		const auto           player = RE::PlayerCharacter::GetSingleton();
		const Game::Snapshot snapshot{
			.quest = g_questResolver.Resolve(player),
			.location = g_locationResolver.Resolve(player),
			.player = Game::ReadPlayerState(player, g_stateMachine.IsSessionActive())
		};

		auto activity = Presence::NormalizeActivity(g_stateMachine.Update(snapshot, g_startTimestamp, now));
		if (!g_stateMachine.IsHoldingActivity() &&
			(!g_hasPublished || !(activity == g_lastPublished)))
		{
			g_lastPublished = activity;
			g_hasPublished = true;
			g_mailbox.Publish(Presence::ActivityUpdate{ std::move(activity) });
		}

		// REX formats before spdlog tests the level, so gate the call itself
		if (!Config::IsDebugLoggingEnabled())
		{
			return;
		}

		REX::DEBUG("#{} presence={} holding={} sessionActive={} quest='{}' objective='{}' priority={} location='{}' worldspace='{}' level={} combatRaw={} combatStable={} mainMenu={} loadingMenu={} looksMenu={} charGenFlag={} nameTrusted={}",
			g_tickState.tickCount,
			Presence::ToString(g_stateMachine.GetState()),
			g_stateMachine.IsHoldingActivity(),
			g_stateMachine.IsSessionActive(),
			snapshot.quest.title,
			snapshot.quest.objective,
			static_cast<int>(snapshot.quest.priority),
			snapshot.location.location,
			snapshot.location.worldspace,
			snapshot.player.level,
			snapshot.player.inCombat,
			g_stateMachine.IsCombatActive(),
			snapshot.player.inMainMenu,
			snapshot.player.inLoadingMenu,
			snapshot.player.inLooksMenu,
			static_cast<int>(snapshot.player.charGenFlag),
			g_stateMachine.IsPlayerNameTrusted());
	}

	// never per-frame: a throwing tick would otherwise flood the log
	void LogTickException(std::string_view a_message) noexcept
	{
		try
		{
			const auto now = Clock::now();
			if (g_tickState.hasLoggedException && now < g_tickState.nextExceptionLog)
			{
				++g_tickState.suppressedExceptions;
				return;
			}

			const auto suppressed = std::exchange(g_tickState.suppressedExceptions, 0);
			g_tickState.hasLoggedException = true;
			g_tickState.nextExceptionLog = now + kExceptionLogInterval;
			REX::ERROR("Tick failed: {} ({} suppressed since last report)", a_message, suppressed);
		}
		catch (...)
		{}
	}

	void Thunk(void* a_manager, RE::NiAVObject* a_object) noexcept
	{
		try
		{
			g_chainedCall(a_manager, a_object);
			RunTick();
		}
		catch (const std::exception& a_exception)
		{
			LogTickException(a_exception.what());
		}
		catch (...)
		{
			LogTickException("unknown exception"sv);
		}
	}
}

namespace Game::Tick
{
	Presence::Mailbox& GetMailbox() noexcept
	{
		return g_mailbox;
	}

	void BeginSession() noexcept
	{
		g_stateMachine.BeginSession();
	}

	void InvalidateCaches()
	{
		g_questResolver.Invalidate();
		g_locationResolver.Invalidate();
	}

	void ResetElapsedEpoch() noexcept
	{
		g_startTimestamp = UnixTimestamp();
		g_tickState.lastSample = {};
		g_hasPublished = false;
	}

	bool IsSupportedRuntime(const REL::Version& a_runtime) noexcept
	{
		return a_runtime == F4SE::RUNTIME_1_10_163 ||
		       a_runtime == F4SE::RUNTIME_1_10_984 ||
		       a_runtime == F4SE::RUNTIME_1_11_221;
	}

	bool Install(const REL::Version& a_runtime)
	{
		if (!IsSupportedRuntime(a_runtime))
		{
			REX::ERROR("Unsupported Fallout 4 runtime {}; tick hook not installed", a_runtime);
			return false;
		}

		if (!HasAddressLibrary(a_runtime))
		{
			REX::ERROR("Address Library for {} not found in Data/F4SE/Plugins; tick hook not installed", a_runtime);
			return false;
		}

		const auto onIdle = Main_OnIdle.address();
		const auto site = onIdle + (a_runtime == F4SE::RUNTIME_1_10_163 ? kOGHookOffset : kNGHookOffset);

		if (!ValidateSite(site))
		{
			REX::ERROR("Tick hook validation failed; runtime={}, Main::OnIdle=0x{:X}, site=0x{:X}, bytes={}",
				a_runtime, onIdle, site, FormatBytes(site));
			return false;
		}

		const auto                      previous = REL::ASM::CALL5::TARGET(site);
		REL::Relocation<std::uintptr_t> callSite{ site };
		g_chainedCall = reinterpret_cast<ChainedCall>(callSite.write_call<5, 0>(&Thunk));

		if (REL::ASM::CALL5::TARGET(site) == previous)
		{
			REX::ERROR("Tick hook write did not take effect at 0x{:X}; tick hook not installed", site);
			g_chainedCall = nullptr;
			return false;
		}

		try
		{
			REX::INFO("Tick hook installed at 0x{:X}; chained target 0x{:X}, trampoline free {} bytes",
				site, reinterpret_cast<std::uintptr_t>(g_chainedCall), REL::GetTrampoline().free_size());
		}
		catch (...)
		{}
		return true;
	}
}
