#pragma once

#include "Presence/Activity.h"

#include <chrono>
#include <cstdint>
#include <optional>
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

	class StateMachine
	{
	public:
		using Clock = std::chrono::steady_clock;

		inline static constexpr auto         kMinimumCharacterCreationSettleDelay = std::chrono::milliseconds{ 200 };
		inline static constexpr std::uint8_t kCombatSampleThreshold = 2;
		inline static constexpr std::uint8_t kLoadingSampleThreshold = 2;

		void                         BeginSession() noexcept;
		[[nodiscard]] ActivityUpdate Update(const Game::Snapshot& a_snapshot, const Config::Snapshot& a_config, std::int64_t a_startTimestamp, Clock::time_point a_now);
		[[nodiscard]] GameState      GetState() const noexcept { return state_; }
		[[nodiscard]] bool           IsPlayerNameTrusted() const noexcept { return state_ != GameState::kCharacterCreation && !nameTrustedAt_; }
		[[nodiscard]] bool           IsSessionActive() const noexcept { return sessionActive_; }
		[[nodiscard]] bool           IsCombatActive() const noexcept { return combatActive_; }
		[[nodiscard]] bool           IsHoldingActivity() const noexcept { return holdActivity_; }

	private:
		[[nodiscard]] GameState DetectState(const Game::Snapshot& a_snapshot);
		[[nodiscard]] Activity  BuildInGame(const Game::Snapshot& a_snapshot, const Config::Snapshot& a_config, std::int64_t a_startTimestamp, bool a_nameTrusted) const;
		void                    EndSession() noexcept;
		void                    UpdateCombat(bool a_inCombat) noexcept;

		GameState                        state_{ GameState::kUnknown };
		std::optional<Clock::time_point> nameTrustedAt_;
		bool                             sessionActive_{ false };
		bool                             awaitingMainMenuClose_{ false };
		bool                             loadingObserved_{ false };
		std::uint8_t                     loadingSamples_{ 0 };
		std::uint8_t                     loadingExitSamples_{ 0 };
		bool                             combatActive_{ false };
		std::uint8_t                     combatTransitionSamples_{ 0 };
		bool                             holdActivity_{ false };
		ActivityUpdate                   lastActivity_;
	};
}
