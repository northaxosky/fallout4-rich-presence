#include "pch.h"

#include "Config.h"

namespace Config
{
	REX::TTomlSetting<std::int32_t> iSamplingIntervalMs{
		"General"sv,
		"iSamplingIntervalMs"sv,
		static_cast<std::int32_t>(kDefaultSamplingInterval.count())
	};
	REX::TTomlSetting<bool> bDebugLogging{ "General"sv, "bDebugLogging"sv, false };

	void Load()
	{
		const auto store = REX::FTomlSettingStore::GetSingleton();
		store->Init(
			"Data/F4SE/Plugins/Fallout4RichPresence.toml",
			"Data/F4SE/Plugins/Fallout4RichPresenceCustom.toml");
		store->Load();

		if (iSamplingIntervalMs.GetValue() <= 0)
		{
			REX::WARN("iSamplingIntervalMs must be positive; using {}", kDefaultSamplingInterval.count());
			iSamplingIntervalMs.SetValue(static_cast<std::int32_t>(kDefaultSamplingInterval.count()));
		}
	}

	std::chrono::milliseconds GetSamplingInterval() noexcept
	{
		return std::chrono::milliseconds{ iSamplingIntervalMs.GetValue() };
	}

	bool IsDebugLoggingEnabled() noexcept
	{
		return bDebugLogging.GetValue();
	}
}
