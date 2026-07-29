#include "pch.h"

namespace
{
	bool InitPlugin(const F4SE::LoadInterface* a_f4se)
	{
		F4SE::Init(a_f4se);
		REX::INFO("{} v{} loaded", PLUGIN_NAME, PLUGIN_VERSION);
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

	return a_f4se->RuntimeVersion() >= REL::Version(F4SE::RUNTIME_1_10_163);
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	return InitPlugin(a_f4se);
}
