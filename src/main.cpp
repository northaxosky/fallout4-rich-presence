#include "pch.h"

#include "Config.h"
#include "Game/Tick.h"

#include <spdlog/spdlog.h>

namespace
{
	// explicit sizing disables FHookStore auto-sizing, so leave room for later hooks
	inline constexpr std::size_t kTrampolineSize = 0x40;

	void ConfigureLogging()
	{
		const auto level = Config::IsDebugLoggingEnabled() ? spdlog::level::debug : spdlog::level::info;
		const auto logger = spdlog::default_logger();
		logger->set_level(level);
		logger->flush_on(level);
	}

	bool InitPlugin(const F4SE::LoadInterface* a_f4se)
	{
		F4SE::Init(a_f4se, { .trampoline = true, .trampolineSize = kTrampolineSize });

		const auto runtime = a_f4se->RuntimeVersion();
		if (!Game::Tick::IsSupportedRuntime(runtime))
		{
			REX::ERROR("Unsupported Fallout 4 runtime {}; plugin disabled", runtime);
			return false;
		}

		Config::Load();
		ConfigureLogging();
		REX::INFO("Sampling every {} ms; debug logging {}",
			Config::GetSamplingInterval().count(),
			Config::IsDebugLoggingEnabled() ? "enabled"sv : "disabled"sv);

		// must stay last: F4SE unloads the DLL when this returns false, and the patch is irreversible
		if (!Game::Tick::Install(runtime))
		{
			REX::ERROR("Tick hook installation failed; plugin disabled");
			return false;
		}

		return true;
	}
}

// OG (1.10.163) resolves the plugin through the query export instead of the version data
F4SE_PLUGIN_QUERY(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	if (const auto data = F4SE::PluginVersionData::GetSingleton())
	{
		a_info->infoVersion = F4SE::PluginInfo::kVersion;
		a_info->name = data->GetPluginName().data();
		a_info->version = data->GetPluginVersion().pack();
	}

	return Game::Tick::IsSupportedRuntime(a_f4se->RuntimeVersion());
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	return InitPlugin(a_f4se);
}
