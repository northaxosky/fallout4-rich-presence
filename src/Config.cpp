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

	// name is personal, quest and location are spoilers; the shipped preset opts into the latter two
	REX::TTomlSetting<bool> bShowPlayerName{ "Privacy"sv, "bShowPlayerName"sv, false };
	REX::TTomlSetting<bool> bShowQuest{ "Privacy"sv, "bShowQuest"sv, true };
	REX::TTomlSetting<bool> bShowLocation{ "Privacy"sv, "bShowLocation"sv, true };

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

	bool ShowPlayerName() noexcept
	{
		return bShowPlayerName.GetValue();
	}

	bool ShowQuest() noexcept
	{
		return bShowQuest.GetValue();
	}

	bool ShowLocation() noexcept
	{
		return bShowLocation.GetValue();
	}
}
