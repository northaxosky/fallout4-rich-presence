#pragma once

#include <REX/TTomlSetting.h>

#include <chrono>
#include <cstdint>

namespace Config
{
	inline constexpr auto kDefaultSamplingInterval = std::chrono::milliseconds{ 500 };

	extern REX::TTomlSetting<std::int32_t> iSamplingIntervalMs;
	extern REX::TTomlSetting<bool>         bDebugLogging;

	void Load();

	[[nodiscard]] std::chrono::milliseconds GetSamplingInterval() noexcept;
	[[nodiscard]] bool                      IsDebugLoggingEnabled() noexcept;
}
