#include "pch.h"

#include "Config.h"
#include "Game/Snapshot.h"
#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"
#include "Presence/StateMachine.h"

#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] Presence::Activity BuildFixedActivity(std::string_view a_details, Presence::Asset a_asset, std::int64_t a_startTimestamp = 0)
	{
		Presence::Activity activity;
		activity.details = a_details;
		activity.largeImage = Config::GetAssetKey(a_asset);
		activity.largeText = "Fallout 4";
		activity.startTimestamp = a_startTimestamp;
		return activity;
	}
}

namespace Presence
{
	void StateMachine::BeginSession() noexcept
	{
		sessionActive_ = true;
		awaitingMainMenuClose_ = true;
		loadingObserved_ = false;
		loadingSamples_ = 0;
		loadingExitSamples_ = 0;
		combatActive_ = false;
		combatTransitionSamples_ = 0;
		holdActivity_ = false;
		nameTrustedAt_.reset();
	}

	void StateMachine::EndSession() noexcept
	{
		sessionActive_ = false;
		awaitingMainMenuClose_ = false;
		loadingObserved_ = false;
		loadingSamples_ = 0;
		loadingExitSamples_ = 0;
		combatActive_ = false;
		combatTransitionSamples_ = 0;
		holdActivity_ = false;
		nameTrustedAt_.reset();
	}

	GameState StateMachine::DetectState(const Game::Snapshot& a_snapshot)
	{
		holdActivity_ = false;
		if (a_snapshot.player.inMainMenu)
		{
			loadingObserved_ = false;
			loadingSamples_ = 0;
			loadingExitSamples_ = 0;
			if (sessionActive_ && !awaitingMainMenuClose_)
			{
				EndSession();
			}
			return GameState::kMainMenu;
		}

		awaitingMainMenuClose_ = false;
		if (a_snapshot.player.inLoadingMenu)
		{
			loadingObserved_ = true;
			loadingExitSamples_ = 0;
			if (loadingSamples_ < kLoadingSampleThreshold)
			{
				++loadingSamples_;
			}

			if (loadingSamples_ >= kLoadingSampleThreshold &&
				(sessionActive_ || state_ == GameState::kMainMenu || state_ == GameState::kLoading))
			{
				return GameState::kLoading;
			}
			holdActivity_ = true;
			return state_;
		}

		loadingSamples_ = 0;
		if (loadingObserved_)
		{
			if (state_ == GameState::kLoading && !sessionActive_)
			{
				holdActivity_ = true;
				return GameState::kLoading;
			}

			if (loadingExitSamples_ < kLoadingSampleThreshold)
			{
				++loadingExitSamples_;
			}
			if (loadingExitSamples_ < kLoadingSampleThreshold)
			{
				holdActivity_ = true;
				return state_;
			}
			loadingObserved_ = false;
		}
		loadingExitSamples_ = 0;

		if (a_snapshot.player.inLooksMenu && sessionActive_)
		{
			return GameState::kCharacterCreation;
		}

		if (sessionActive_)
		{
			return GameState::kInGame;
		}

		return GameState::kUnknown;
	}

	void StateMachine::UpdateCombat(bool a_inCombat) noexcept
	{
		if (a_inCombat == combatActive_)
		{
			combatTransitionSamples_ = 0;
			return;
		}

		if (combatTransitionSamples_ < kCombatSampleThreshold)
		{
			++combatTransitionSamples_;
		}

		if (combatTransitionSamples_ >= kCombatSampleThreshold)
		{
			combatActive_ = a_inCombat;
			combatTransitionSamples_ = 0;
		}
	}

	Activity StateMachine::BuildInGame(const Game::Snapshot& a_snapshot, std::int64_t a_startTimestamp, bool a_nameTrusted) const
	{
		Activity activity;
		activity.largeImage = Config::GetAssetKey(Asset::kFallout4);
		activity.startTimestamp = a_startTimestamp;

		const auto showName = Config::ShowPlayerName() && a_nameTrusted;
		const auto showQuest = Config::ShowQuest();
		const auto showLocation = Config::ShowLocation();
		const auto showExactLocation = showLocation && Config::ShowExactLocation();
		const auto level = a_snapshot.player.level > 0 ? std::to_string(a_snapshot.player.level) : std::string{};
		const auto state = combatActive_ ? "In Combat"sv : "In Game"sv;

		const FormatValues values{
			.name = showName ? std::string_view{ a_snapshot.player.name } : std::string_view{},
			.level = level,
			.quest = showQuest && a_snapshot.quest.hasQuest ? std::string_view{ a_snapshot.quest.title } : std::string_view{},
			.objective = showQuest && a_snapshot.quest.hasQuest && a_snapshot.quest.hasObjective ? std::string_view{ a_snapshot.quest.objective } : std::string_view{},
			.location = showExactLocation ? std::string_view{ a_snapshot.location.location } : std::string_view{},
			.worldspace = showLocation ? std::string_view{ a_snapshot.location.worldspace } : std::string_view{},
			.state = state
		};

		activity.details = Config::GetDetailsTemplate().Render(values);
		activity.state = Config::GetStateTemplate().Render(values);
		activity.largeText = Config::GetLargeTextTemplate().Render(values);
		if (activity.details.empty() && activity.state.empty())
		{
			activity.state = level.empty() ? "In Game" : "Level " + level;
		}

		if (combatActive_)
		{
			activity.smallImage = Config::GetAssetKey(Asset::kCombat);
			activity.smallText = Config::GetCombatSmallTextTemplate().Render(values);
		}
		else
		{
			activity.smallText = Config::GetSmallTextTemplate().Render(values);
			if (!activity.smallText.empty())
			{
				activity.smallImage = Config::GetAssetKey(Asset::kPlayer);
			}
		}

		return activity;
	}

	ActivityUpdate StateMachine::Update(const Game::Snapshot& a_snapshot, std::int64_t a_startTimestamp, Clock::time_point a_now)
	{
		const auto nextState = DetectState(a_snapshot);
		if (state_ == GameState::kCharacterCreation && nextState != GameState::kCharacterCreation)
		{
			nameTrustedAt_ = a_now + kMinimumCharacterCreationSettleDelay;
		}
		else if (nextState == GameState::kCharacterCreation)
		{
			nameTrustedAt_.reset();
		}

		if (nameTrustedAt_ && a_now >= *nameTrustedAt_)
		{
			nameTrustedAt_.reset();
		}

		state_ = nextState;
		if (holdActivity_)
		{
			return lastActivity_;
		}

		if (state_ == GameState::kInGame)
		{
			UpdateCombat(a_snapshot.player.inCombat);
		}

		ActivityUpdate activity;
		switch (state_)
		{
			case GameState::kUnknown:
				break;
			case GameState::kMainMenu:
				activity = BuildFixedActivity("Main Menu"sv, Asset::kMainMenu);
				break;
			case GameState::kLoading:
				activity = BuildFixedActivity("Loading"sv, Asset::kLoading);
				break;
			case GameState::kCharacterCreation:
				activity = BuildFixedActivity("Character Creation"sv, Asset::kCharacterCreation, a_startTimestamp);
				break;
			case GameState::kInGame:
				activity = BuildInGame(a_snapshot, a_startTimestamp, IsPlayerNameTrusted());
				break;
		}

		lastActivity_ = activity;
		return activity;
	}
}
