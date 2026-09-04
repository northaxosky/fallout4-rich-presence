#include "pch.h"

#include "Config.h"
#include "Game/Snapshot.h"
#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"
#include "Presence/MarkerAssets.h"
#include "Presence/StateMachine.h"

#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] Presence::Activity BuildFixedActivity(
		std::string_view        a_details,
		Presence::Asset         a_asset,
		const Config::Snapshot& a_config,
		std::int64_t            a_startTimestamp = 0)
	{
		Presence::Activity activity;
		activity.details = a_details;
		activity.largeImage = a_config.GetAssetKey(a_asset);
		activity.largeText = a_config.labelGameTitle;
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
		combatActive_.Reset();
		combatTarget_.Reset();
		menuActivity_.Reset();
		powerArmorActive_.Reset();
		irradiatedActive_.Reset();
		stateBadge_ = StateBadge::kNone;
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
		combatActive_.Reset();
		combatTarget_.Reset();
		menuActivity_.Reset();
		powerArmorActive_.Reset();
		irradiatedActive_.Reset();
		stateBadge_ = StateBadge::kNone;
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

	void StateMachine::UpdateStateBadge(
		const Game::Snapshot&   a_snapshot,
		const Config::Snapshot& a_config)
	{
		combatActive_.Update(a_snapshot.player.inCombat);
		if (combatActive_.IsActive())
		{
			combatTarget_.Update(CombatTarget{
				.id = a_snapshot.player.combatTargetID,
				.name = a_snapshot.player.combatTargetName });
		}

		else
		{
			combatTarget_.Reset();
		}
		powerArmorActive_.Update(a_snapshot.player.inPowerArmor);
		const auto irradiatedThreshold = static_cast<float>(a_config.irradiatedPercent) / 100.0F;
		irradiatedActive_.Update(a_snapshot.player.radsFraction >= irradiatedThreshold);
		stateBadge_ = ResolveStateBadge(
			combatActive_.IsActive(),
			a_config.stateBadge && powerArmorActive_.IsActive(),
			a_config.stateBadge && irradiatedActive_.IsActive());
	}

	void StateMachine::UpdateMenuActivity(const Game::Snapshot& a_snapshot)
	{
		const auto rawActivity = static_cast<MenuActivity>(a_snapshot.player.menuActivity);
		if (rawActivity == MenuActivity::kNone)
		{
			menuActivity_.Reset();
			return;
		}

		menuActivity_.Update(MenuActivityValue{
			.activity = rawActivity,
			.name = a_snapshot.player.menuActivityName });
	}

	Activity StateMachine::BuildInGame(
		const Game::Snapshot&   a_snapshot,
		const Config::Snapshot& a_config,
		std::int64_t            a_startTimestamp,
		bool                    a_nameTrusted) const
	{
		Activity activity;
		activity.largeImage = a_config.GetAssetKey(Asset::kFallout4);
		if (a_config.showLocation && a_config.markerArtwork && a_snapshot.markerType)
		{
			const auto markerKey = MarkerAssetKey(*a_snapshot.markerType);
			if (!markerKey.empty())
			{
				activity.largeImage = markerKey;
			}
		}
		activity.startTimestamp = a_startTimestamp;

		const auto             showName = a_config.showPlayerName && a_nameTrusted;
		const auto             showQuest = a_config.showQuest;
		const auto             showLocation = a_config.showLocation;
		const auto             showExactLocation = showLocation && a_config.showExactLocation;
		const auto             level = a_snapshot.player.level > 0 ? std::to_string(a_snapshot.player.level) : std::string{};
		const StateBadgeLabels labels{
			.inGame = a_config.labelInGame,
			.inCombat = a_config.labelInCombat,
			.inPowerArmor = a_config.labelInPowerArmor,
			.irradiated = a_config.labelIrradiated
		};
		const auto               state = StateBadgeLabel(stateBadge_, labels);
		const MenuActivityLabels activityLabels{
			.barter = a_config.labelBarter,
			.workbench = a_config.labelWorkbench,
			.workshop = a_config.labelWorkshop,
			.terminal = a_config.labelTerminal,
			.lockpicking = a_config.labelLockpicking,
			.sitWait = a_config.labelSitWait,
			.dialogue = a_config.labelDialogue
		};
		const auto  menuActivity = GetMenuActivity();
		std::string activityText;
		if (a_config.showMenuActivity && menuActivity != MenuActivity::kNone)
		{
			const auto activityName = GetMenuActivityName();
			if (!activityName.empty() && menuActivity == MenuActivity::kBarter)
			{
				activityText = a_config.labelBarterNamed.Render(FormatValues{ .name = activityName });
			}
			else if (!activityName.empty() && menuActivity == MenuActivity::kWorkbench)
			{
				activityText = a_config.labelWorkbenchNamed.Render(FormatValues{ .name = activityName });
			}
			else
			{
				activityText = MenuActivityLabel(menuActivity, activityLabels);
			}
		}

		const FormatValues values{
			.name = showName ? std::string_view{ a_snapshot.player.name } : std::string_view{},
			.level = level,
			.quest = showQuest && a_snapshot.quest.hasQuest ? std::string_view{ a_snapshot.quest.title } : std::string_view{},
			.objective = showQuest && a_snapshot.quest.hasQuest && a_snapshot.quest.hasObjective ? std::string_view{ a_snapshot.quest.objective } : std::string_view{},
			.location = showExactLocation ? std::string_view{ a_snapshot.location.location } : std::string_view{},
			.worldspace = showLocation ? std::string_view{ a_snapshot.location.worldspace } : std::string_view{},
			.state = state,
			.target = a_config.showCombatTarget && combatActive_.IsActive() ?
			              GetCombatTargetName() :
			              std::string_view{},
			.activity = activityText
		};

		activity.details = a_config.details.Render(values);
		activity.state = a_config.state.Render(values);
		activity.largeText = a_config.largeText.Render(values);
		if (activity.details.empty() && activity.state.empty())
		{
			activity.state = level.empty() ? a_config.labelInGame : a_config.labelLevel.Render(values);
		}
		if (a_config.showMenuActivity && menuActivity != MenuActivity::kNone)
		{
			activity.details = activityText;
		}

		switch (stateBadge_)
		{
			case StateBadge::kCombat:
				activity.smallImage = a_config.GetAssetKey(Asset::kCombat);
				activity.smallText = a_config.combatSmallText.Render(values);
				// an all-token template renders empty without a target name, leaving the badge untitled
				if (activity.smallText.empty())
				{
					activity.smallText = StateBadgeLabel(StateBadge::kCombat, labels);
				}
				break;
			case StateBadge::kPowerArmor:
				activity.smallImage = a_config.GetAssetKey(Asset::kPowerArmor);
				activity.smallText = a_config.smallText.Render(values);
				break;
			case StateBadge::kIrradiated:
				activity.smallImage = a_config.GetAssetKey(Asset::kIrradiated);
				activity.smallText = a_config.smallText.Render(values);
				break;
			case StateBadge::kNone:
			{
				activity.smallText = a_config.smallText.Render(values);
				if (!activity.smallText.empty())
				{
					activity.smallImage = a_config.GetAssetKey(Asset::kPlayer);
				}
				break;
			}
		}

		// a badge identical to the large image reads as a duplicate; drops out on its own
		// once distinct art is configured
		if (activity.smallImage == activity.largeImage)
		{
			activity.smallImage.clear();
		}

		// small text is only a tooltip for the badge
		if (activity.smallImage.empty())
		{
			activity.smallText.clear();
		}

		return activity;
	}

	ActivityUpdate StateMachine::Update(
		const Game::Snapshot&   a_snapshot,
		const Config::Snapshot& a_config,
		std::int64_t            a_startTimestamp,
		Clock::time_point       a_now)
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
			UpdateMenuActivity(a_snapshot);
			UpdateStateBadge(a_snapshot, a_config);
		}
		else
		{
			menuActivity_.Reset();
		}

		ActivityUpdate activity;
		switch (state_)
		{
			case GameState::kUnknown:
				break;
			case GameState::kMainMenu:
				activity = BuildFixedActivity(a_config.labelMainMenu, Asset::kMainMenu, a_config);
				break;
			case GameState::kLoading:
				activity = BuildFixedActivity(a_config.labelLoading, Asset::kLoading, a_config);
				break;
			case GameState::kCharacterCreation:
				activity = BuildFixedActivity(a_config.labelCharacterCreation, Asset::kCharacterCreation, a_config, a_startTimestamp);
				break;
			case GameState::kInGame:
				activity = BuildInGame(a_snapshot, a_config, a_startTimestamp, IsPlayerNameTrusted());
				break;
		}

		lastActivity_ = activity;
		return activity;
	}
}
