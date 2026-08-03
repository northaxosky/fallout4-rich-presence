#pragma once

#include <REX/TTomlSetting.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace Presence
{
	enum class Asset : std::uint8_t;
	class FormatTemplate;
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

	void Load();

	[[nodiscard]] std::chrono::milliseconds       GetSamplingInterval() noexcept;
	[[nodiscard]] bool                            IsDebugLoggingEnabled() noexcept;
	[[nodiscard]] bool                            ShowPlayerName() noexcept;
	[[nodiscard]] bool                            ShowQuest() noexcept;
	[[nodiscard]] bool                            ShowLocation() noexcept;
	[[nodiscard]] bool                            ShowExactLocation() noexcept;
	[[nodiscard]] std::string                     GetApplicationID();
	[[nodiscard]] std::string_view                GetAssetKey(Presence::Asset a_asset) noexcept;
	[[nodiscard]] const Presence::FormatTemplate& GetDetailsTemplate() noexcept;
	[[nodiscard]] const Presence::FormatTemplate& GetStateTemplate() noexcept;
	[[nodiscard]] const Presence::FormatTemplate& GetLargeTextTemplate() noexcept;
	[[nodiscard]] const Presence::FormatTemplate& GetSmallTextTemplate() noexcept;
	[[nodiscard]] const Presence::FormatTemplate& GetCombatSmallTextTemplate() noexcept;
}
