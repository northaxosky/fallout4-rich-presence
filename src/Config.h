#pragma once

#include <REX/TTomlSetting.h>

#include <chrono>
#include <cstdint>

namespace Config
{
	inline constexpr auto kDefaultSamplingInterval = std::chrono::milliseconds{ 500 };

	extern REX::TTomlSetting<std::int32_t> iSamplingIntervalMs;
	extern REX::TTomlSetting<bool>         bDebugLogging;
	extern REX::TTomlSetting<bool>         bShowPlayerName;
	extern REX::TTomlSetting<bool>         bShowQuest;
	extern REX::TTomlSetting<bool>         bShowLocation;

	void Load();

	[[nodiscard]] std::chrono::milliseconds GetSamplingInterval() noexcept;
	[[nodiscard]] bool                      IsDebugLoggingEnabled() noexcept;
	[[nodiscard]] bool                      ShowPlayerName() noexcept;
	[[nodiscard]] bool                      ShowQuest() noexcept;
	[[nodiscard]] bool                      ShowLocation() noexcept;
}
