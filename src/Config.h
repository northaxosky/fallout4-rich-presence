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
	inline constexpr std::string_view kDefaultDetailsTemplate = "{quest}";
	inline constexpr std::string_view kDefaultStateTemplate = "{location} - {worldspace}";
	inline constexpr std::string_view kDefaultLargeTextTemplate = "{objective}";
	inline constexpr std::string_view kDefaultSmallTextTemplate = "{name} - Level {level}";
	inline constexpr std::string_view kDefaultCombatSmallTextTemplate = "{state}";

	extern REX::TTomlSetting<std::int32_t> iSamplingIntervalMs;
	extern REX::TTomlSetting<bool>         bDebugLogging;
	extern REX::TTomlSetting<bool>         bShowPlayerName;
	extern REX::TTomlSetting<bool>         bShowQuest;
	extern REX::TTomlSetting<bool>         bShowLocation;
	extern REX::TTomlSetting<bool>         bShowExactLocation;
	extern REX::TTomlSetting<std::string>  sApplicationID;
	extern REX::TTomlSetting<std::string>  sAssetDefault;
	extern REX::TTomlSetting<std::string>  sAssetMainMenu;
	extern REX::TTomlSetting<std::string>  sAssetLoading;
	extern REX::TTomlSetting<std::string>  sAssetCharacterCreation;
	extern REX::TTomlSetting<std::string>  sAssetPlayer;
	extern REX::TTomlSetting<std::string>  sAssetCombat;
	extern REX::TTomlSetting<std::string>  sDetails;
	extern REX::TTomlSetting<std::string>  sState;
	extern REX::TTomlSetting<std::string>  sLargeText;
	extern REX::TTomlSetting<std::string>  sSmallText;
	extern REX::TTomlSetting<std::string>  sCombatSmallText;

	struct Snapshot
	{
		std::chrono::milliseconds  samplingInterval{ kDefaultSamplingInterval };
		bool                       debugLogging{ false };
		bool                       showPlayerName{ false };
		bool                       showQuest{ true };
		bool                       showLocation{ true };
		bool                       showExactLocation{ true };
		std::array<std::string, 6> assetKeys{};
		Presence::FormatTemplate   details{};
		Presence::FormatTemplate   state{};
		Presence::FormatTemplate   largeText{};
		Presence::FormatTemplate   smallText{};
		Presence::FormatTemplate   combatSmallText{};

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
