#pragma once

#include "Presence/FormatTemplate.h"

#include <REX/TTomlSetting.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Presence
{
	enum class Asset : std::uint8_t;
}

namespace Config
{
	inline constexpr auto             kDefaultSamplingInterval = std::chrono::milliseconds{ 500 };
	inline constexpr std::int32_t     kDefaultIrradiatedPercent = 25;
	inline constexpr std::int32_t     kMinimumIrradiatedPercent = 1;
	inline constexpr std::int32_t     kMaximumIrradiatedPercent = 100;
	inline constexpr std::int32_t     kDefaultMarkerMaxDistance = 16384;
	inline constexpr std::int32_t     kMinimumMarkerMaxDistance = 1;
	inline constexpr std::int32_t     kMaximumMarkerMaxDistance = 131072;
	inline constexpr std::string_view kDefaultDetailsTemplate = "{quest}";
	inline constexpr std::string_view kDefaultStateTemplate = "{location} - {worldspace}";
	inline constexpr std::string_view kDefaultLargeTextTemplate = "{objective}";
	inline constexpr std::string_view kDefaultSmallTextTemplate = "{name} - Level {level}";
	inline constexpr std::string_view kDefaultCombatSmallTextTemplate = "Fighting {target}";
	inline constexpr std::string_view kDefaultLabelMainMenu = "Main Menu";
	inline constexpr std::string_view kDefaultLabelLoading = "Loading";
	inline constexpr std::string_view kDefaultLabelCharacterCreation = "Character Creation";
	inline constexpr std::string_view kDefaultLabelGameTitle = "Fallout 4";
	inline constexpr std::string_view kDefaultLabelInGame = "In Game";
	inline constexpr std::string_view kDefaultLabelInCombat = "In Combat";
	inline constexpr std::string_view kDefaultLabelInPowerArmor = "In Power Armor";
	inline constexpr std::string_view kDefaultLabelIrradiated = "Irradiated";
	inline constexpr std::string_view kDefaultLabelLevelTemplate = "Level {level}";
	inline constexpr std::string_view kDefaultLabelBarter = "Trading";
	inline constexpr std::string_view kDefaultLabelBarterNamedTemplate = "Trading with {name}";
	inline constexpr std::string_view kDefaultLabelWorkbench = "Using a Workbench";
	inline constexpr std::string_view kDefaultLabelWorkbenchNamedTemplate = "Using the {name}";
	inline constexpr std::string_view kDefaultLabelWorkshop = "Building";
	inline constexpr std::string_view kDefaultLabelTerminal = "Using a Terminal";
	inline constexpr std::string_view kDefaultLabelLockpicking = "Lockpicking";
	inline constexpr std::string_view kDefaultLabelSitWait = "Waiting";
	inline constexpr std::string_view kDefaultLabelDialogue = "Talking";

	extern REX::TTomlSetting<std::int32_t> iSamplingIntervalMs;
	extern REX::TTomlSetting<std::int32_t> iIrradiatedPercent;
	extern REX::TTomlSetting<bool>         bDebugLogging;
	extern REX::TTomlSetting<bool>         bShowPlayerName;
	extern REX::TTomlSetting<bool>         bShowQuest;
	extern REX::TTomlSetting<bool>         bShowLocation;
	extern REX::TTomlSetting<bool>         bShowExactLocation;
	extern REX::TTomlSetting<bool>         bShowCombatTarget;
	extern REX::TTomlSetting<bool>         bShowMenuActivity;
	extern REX::TTomlSetting<std::string>  sApplicationID;
	extern REX::TTomlSetting<bool>         bMarkerArtwork;
	extern REX::TTomlSetting<bool>         bStateBadge;
	extern REX::TTomlSetting<std::int32_t> iMarkerMaxDistance;
	extern REX::TTomlSetting<std::string>  sAssetDefault;
	extern REX::TTomlSetting<std::string>  sAssetMainMenu;
	extern REX::TTomlSetting<std::string>  sAssetLoading;
	extern REX::TTomlSetting<std::string>  sAssetCharacterCreation;
	extern REX::TTomlSetting<std::string>  sAssetPlayer;
	extern REX::TTomlSetting<std::string>  sAssetCombat;
	extern REX::TTomlSetting<std::string>  sAssetPowerArmor;
	extern REX::TTomlSetting<std::string>  sAssetIrradiated;
	extern REX::TTomlSetting<std::string>  sDetails;
	extern REX::TTomlSetting<std::string>  sState;
	extern REX::TTomlSetting<std::string>  sLargeText;
	extern REX::TTomlSetting<std::string>  sSmallText;
	extern REX::TTomlSetting<std::string>  sCombatSmallText;
	extern REX::TTomlSetting<std::string>  sLabelMainMenu;
	extern REX::TTomlSetting<std::string>  sLabelLoading;
	extern REX::TTomlSetting<std::string>  sLabelCharacterCreation;
	extern REX::TTomlSetting<std::string>  sLabelGameTitle;
	extern REX::TTomlSetting<std::string>  sLabelInGame;
	extern REX::TTomlSetting<std::string>  sLabelInCombat;
	extern REX::TTomlSetting<std::string>  sLabelInPowerArmor;
	extern REX::TTomlSetting<std::string>  sLabelIrradiated;
	extern REX::TTomlSetting<std::string>  sLabelLevel;
	extern REX::TTomlSetting<std::string>  sLabelBarter;
	extern REX::TTomlSetting<std::string>  sLabelBarterNamed;
	extern REX::TTomlSetting<std::string>  sLabelWorkbench;
	extern REX::TTomlSetting<std::string>  sLabelWorkbenchNamed;
	extern REX::TTomlSetting<std::string>  sLabelWorkshop;
	extern REX::TTomlSetting<std::string>  sLabelTerminal;
	extern REX::TTomlSetting<std::string>  sLabelLockpicking;
	extern REX::TTomlSetting<std::string>  sLabelSitWait;
	extern REX::TTomlSetting<std::string>  sLabelDialogue;

	struct Snapshot
	{
		std::chrono::milliseconds  samplingInterval{ kDefaultSamplingInterval };
		std::int32_t               irradiatedPercent{ kDefaultIrradiatedPercent };
		bool                       debugLogging{ false };
		bool                       showPlayerName{ false };
		bool                       showQuest{ true };
		bool                       showLocation{ true };
		bool                       showExactLocation{ true };
		bool                       showCombatTarget{ true };
		bool                       showMenuActivity{ true };
		bool                       markerArtwork{ true };
		bool                       stateBadge{ true };
		std::int32_t               markerMaxDistance{ kDefaultMarkerMaxDistance };
		std::array<std::string, 8> assetKeys{};
		Presence::FormatTemplate   details{};
		Presence::FormatTemplate   state{};
		Presence::FormatTemplate   largeText{};
		Presence::FormatTemplate   smallText{};
		Presence::FormatTemplate   combatSmallText{};
		std::string                labelMainMenu{ kDefaultLabelMainMenu };
		std::string                labelLoading{ kDefaultLabelLoading };
		std::string                labelCharacterCreation{ kDefaultLabelCharacterCreation };
		std::string                labelGameTitle{ kDefaultLabelGameTitle };
		std::string                labelInGame{ kDefaultLabelInGame };
		std::string                labelInCombat{ kDefaultLabelInCombat };
		std::string                labelInPowerArmor{ kDefaultLabelInPowerArmor };
		std::string                labelIrradiated{ kDefaultLabelIrradiated };
		Presence::FormatTemplate   labelLevel{};
		std::string                labelBarter{ kDefaultLabelBarter };
		Presence::FormatTemplate   labelBarterNamed{};
		std::string                labelWorkbench{ kDefaultLabelWorkbench };
		Presence::FormatTemplate   labelWorkbenchNamed{};
		std::string                labelWorkshop{ kDefaultLabelWorkshop };
		std::string                labelTerminal{ kDefaultLabelTerminal };
		std::string                labelLockpicking{ kDefaultLabelLockpicking };
		std::string                labelSitWait{ kDefaultLabelSitWait };
		std::string                labelDialogue{ kDefaultLabelDialogue };

		[[nodiscard]] std::string_view GetAssetKey(Presence::Asset a_asset) const noexcept;
	};

	// a live edit is transient, so it must not warn or rewrite the value the user is still typing
	enum class Validation : std::uint8_t
	{
		kQuiet,
		kStrict
	};

	void Load();
	void Rebuild(Validation a_validation = Validation::kStrict);

	[[nodiscard]] std::shared_ptr<const Snapshot> Current() noexcept;
	[[nodiscard]] std::string                     GetApplicationID();
	[[nodiscard]] bool                            SaveOverrides();
}
