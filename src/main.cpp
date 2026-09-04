#include "pch.h"

#include "Config.h"
#include "Discord/Worker.h"
#include "Game/Conflicts.h"
#include "Game/Tick.h"
#include "Host/Client.h"
#include "Logging.h"

#include <REX/FModule.h>
#include <algorithm>
#include <exception>
#include <string_view>
#include <utility>

namespace
{
	// explicit sizing disables FHookStore auto-sizing, so leave room for later hooks
	inline constexpr std::size_t kTrampolineSize = 0x40;

	[[nodiscard]] bool IsPlausibleApplicationID(std::string_view a_value) noexcept
	{
		return a_value.size() >= 17 &&
		       a_value.size() <= 20 &&
		       std::ranges::all_of(a_value, [](char a_character) { return a_character >= '0' && a_character <= '9'; });
	}

	void LogStartupDiagnostics(const REL::Version& a_runtime) noexcept
	{
		try
		{
			const auto moduleBase = REX::FModule::GetExecutingModule().GetBaseAddress();
			const auto hasAddressLibrary = Game::Tick::HasAddressLibrary(a_runtime);
			REX::INFO("{} v{} startup: Fallout 4 runtime {}", PLUGIN_NAME, PLUGIN_VERSION, a_runtime);
			REX::INFO(
				"Fallout4.exe base=0x{:X}; Address Library version-{}.bin {}",
				moduleBase,
				a_runtime.string("-"),
				hasAddressLibrary ? "present"sv : "missing"sv);

			if (hasAddressLibrary)
			{
				const auto onIdle = Game::Tick::GetMainOnIdleAddress();
				REX::INFO("Main::OnIdle=0x{:X} (RVA 0x{:X})", onIdle, onIdle - moduleBase);
			}
			else
			{
				REX::INFO("Main::OnIdle unresolved because the Address Library is unavailable");
			}
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::WARN("Startup diagnostics unavailable: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::WARN("Startup diagnostics unavailable");
			}
			catch (...)
			{}
		}
	}

	void DetectAndReportConflicts() noexcept
	{
		try
		{
			auto conflicts = Game::DetectConflicts();
			for (const auto& conflict : conflicts)
			{
				REX::WARN(
					"Conflicting plugin {} ({}) is loaded; running both can duplicate or flicker Discord presence. Disable one of the two mods.",
					conflict.module,
					conflict.displayName);
			}
			Host::SetConflicts(std::move(conflicts));
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::WARN("Conflicting-plugin detection failed: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::WARN("Conflicting-plugin detection failed");
			}
			catch (...)
			{}
		}
	}

	void BuildMarkerCache() noexcept
	{
		try
		{
			Game::Tick::BuildMarkerCache();
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::ERROR("Map marker cache build failed: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::ERROR("Map marker cache build failed");
			}
			catch (...)
			{}
		}
	}

	void F4SEAPI HandleF4SEMessage(F4SE::MessagingInterface::Message* a_message) noexcept
	{
		if (!a_message)
		{
			return;
		}

		if (a_message->type == F4SE::MessagingInterface::kPostPostLoad)
		{
			Host::Connect();
			return;
		}

		if (a_message->type == F4SE::MessagingInterface::kGameDataReady)
		{
			BuildMarkerCache();
			return;
		}

		if (a_message->type != F4SE::MessagingInterface::kPostLoadGame &&
			a_message->type != F4SE::MessagingInterface::kNewGame)
		{
			return;
		}

		// F4SE encodes the post-load success bool in the pointer value
		if (a_message->type == F4SE::MessagingInterface::kPostLoadGame &&
			(a_message->dataLen != 1 || !a_message->data))
		{
			return;
		}

		try
		{
			Game::Tick::InvalidateCaches();
			Game::Tick::BeginSession();
			Game::Tick::ResetElapsedEpoch();
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::ERROR("Game-session reset failed: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::ERROR("Game-session reset failed");
			}
			catch (...)
			{}
		}
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
		Logging::Configure();
		LogStartupDiagnostics(runtime);
		DetectAndReportConflicts();
		const auto config = Config::Current();
		REX::INFO("Sampling every {} ms; debug logging {}",
			config->samplingInterval.count(),
			config->debugLogging ? "enabled"sv : "disabled"sv);

		auto       applicationID = Config::GetApplicationID();
		const auto validApplicationID = IsPlausibleApplicationID(applicationID);
		if (!validApplicationID)
		{
			REX::WARN("sApplicationID must contain 17-20 decimal digits; Discord transport disabled");
		}

		if (!Game::Tick::Install(runtime))
		{
			REX::ERROR("Tick hook installation failed; plugin disabled");
			return false;
		}

		// Irreversible: no failure below unloads the DLL
		auto listenerRegistered = false;
		try
		{
			const auto messaging = F4SE::GetMessagingInterface();
			listenerRegistered = messaging && messaging->RegisterListener(&HandleF4SEMessage);
		}
		catch (...)
		{}

		if (!listenerRegistered)
		{
			REX::ERROR("F4SE messaging listener registration failed; game-session presence disabled");
		}

		if (validApplicationID && !Discord::Worker::Start(Game::Tick::GetMailbox(), std::move(applicationID)))
		{
			REX::ERROR("Discord worker failed to start");
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
