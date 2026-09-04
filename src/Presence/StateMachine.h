#pragma once

#include "Presence/Activity.h"
#include "Presence/StateBadge.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Game
{
	struct Snapshot;
}

namespace Config
{
	struct Snapshot;
}

namespace Presence
{
	enum class GameState : std::uint8_t
	{
		kUnknown,
		kMainMenu,
		kLoading,
		kCharacterCreation,
		kInGame
	};

	[[nodiscard]] constexpr std::string_view ToString(GameState a_state) noexcept
	{
		switch (a_state)
		{
			case GameState::kUnknown:
				return "unknown";
			case GameState::kMainMenu:
				return "main_menu";
			case GameState::kLoading:
				return "loading";
			case GameState::kCharacterCreation:
				return "character_creation";
			case GameState::kInGame:
				return "in_game";
		}

		return "unknown";
	}

	struct CombatTarget
	{
		std::uint32_t id{ 0 };
		std::string   name;

		[[nodiscard]] bool operator==(const CombatTarget& a_rhs) const noexcept
		{
			return id == a_rhs.id;
		}
	};

	class StateMachine
	{
	public:
		using Clock = std::chrono::steady_clock;

		inline static constexpr auto         kMinimumCharacterCreationSettleDelay = std::chrono::milliseconds{ 200 };
		inline static constexpr std::uint8_t kCombatSampleThreshold = kStateBadgeSampleThreshold;
		inline static constexpr std::uint8_t kLoadingSampleThreshold = 2;

		void                           BeginSession() noexcept;
		[[nodiscard]] ActivityUpdate   Update(const Game::Snapshot& a_snapshot, const Config::Snapshot& a_config, std::int64_t a_startTimestamp, Clock::time_point a_now);
		[[nodiscard]] GameState        GetState() const noexcept { return state_; }
		[[nodiscard]] bool             IsPlayerNameTrusted() const noexcept { return state_ != GameState::kCharacterCreation && !nameTrustedAt_; }
		[[nodiscard]] bool             IsSessionActive() const noexcept { return sessionActive_; }
		[[nodiscard]] bool             IsCombatActive() const noexcept { return combatActive_.IsActive(); }
		[[nodiscard]] std::uint32_t    GetCombatTargetID() const noexcept { return combatTarget_.Get().id; }
		[[nodiscard]] std::string_view GetCombatTargetName() const noexcept { return combatTarget_.Get().name; }
		[[nodiscard]] StateBadge       GetStateBadge() const noexcept { return stateBadge_; }
		[[nodiscard]] bool             IsHoldingActivity() const noexcept { return holdActivity_; }

	private:
		[[nodiscard]] GameState DetectState(const Game::Snapshot& a_snapshot);
		[[nodiscard]] Activity  BuildInGame(const Game::Snapshot& a_snapshot, const Config::Snapshot& a_config, std::int64_t a_startTimestamp, bool a_nameTrusted) const;
		void                    EndSession() noexcept;
		void                    UpdateStateBadge(const Game::Snapshot& a_snapshot, const Config::Snapshot& a_config);

		GameState                        state_{ GameState::kUnknown };
		std::optional<Clock::time_point> nameTrustedAt_;
		bool                             sessionActive_{ false };
		bool                             awaitingMainMenuClose_{ false };
		bool                             loadingObserved_{ false };
		std::uint8_t                     loadingSamples_{ 0 };
		std::uint8_t                     loadingExitSamples_{ 0 };
		DebouncedFlag                    combatActive_;
		DebouncedValue<CombatTarget>     combatTarget_;
		DebouncedFlag                    powerArmorActive_;
		DebouncedFlag                    irradiatedActive_;
		StateBadge                       stateBadge_{ StateBadge::kNone };
		bool                             holdActivity_{ false };
		ActivityUpdate                   lastActivity_;
	};
}
