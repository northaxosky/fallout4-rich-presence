#include "pch.h"

#include "Config.h"
#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <toml.hpp>

namespace Config
{
	REX::TTomlSetting<std::int32_t> iSamplingIntervalMs{
		"General"sv,
		"iSamplingIntervalMs"sv,
		static_cast<std::int32_t>(kDefaultSamplingInterval.count())
	};
	REX::TTomlSetting<std::int32_t> iIrradiatedPercent{
		"General"sv,
		"iIrradiatedPercent"sv,
		kDefaultIrradiatedPercent
	};
	REX::TTomlSetting<bool> bDebugLogging{ "General"sv, "bDebugLogging"sv, false };

	// name is personal; quest, location, and combat targets are spoilers
	REX::TTomlSetting<bool>        bShowPlayerName{ "Privacy"sv, "bShowPlayerName"sv, false };
	REX::TTomlSetting<bool>        bShowQuest{ "Privacy"sv, "bShowQuest"sv, true };
	REX::TTomlSetting<bool>        bShowLocation{ "Privacy"sv, "bShowLocation"sv, true };
	REX::TTomlSetting<bool>        bShowExactLocation{ "Privacy"sv, "bShowExactLocation"sv, true };
	REX::TTomlSetting<bool>        bShowCombatTarget{ "Privacy"sv, "bShowCombatTarget"sv, true };
	REX::TTomlSetting<std::string> sApplicationID{ "Discord"sv, "sApplicationID"sv, "1533687297684537374" };

	REX::TTomlSetting<bool>         bMarkerArtwork{ "Assets"sv, "bMarkerArtwork"sv, true };
	REX::TTomlSetting<bool>         bStateBadge{ "Assets"sv, "bStateBadge"sv, true };
	REX::TTomlSetting<std::int32_t> iMarkerMaxDistance{
		"Assets"sv,
		"iMarkerMaxDistance"sv,
		kDefaultMarkerMaxDistance
	};
	REX::TTomlSetting<std::string> sAssetDefault{ "Assets"sv, "sAssetDefault"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kFallout4) } };
	REX::TTomlSetting<std::string> sAssetMainMenu{ "Assets"sv, "sAssetMainMenu"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kMainMenu) } };
	REX::TTomlSetting<std::string> sAssetLoading{ "Assets"sv, "sAssetLoading"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kLoading) } };
	REX::TTomlSetting<std::string> sAssetCharacterCreation{ "Assets"sv, "sAssetCharacterCreation"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kCharacterCreation) } };
	REX::TTomlSetting<std::string> sAssetPlayer{ "Assets"sv, "sAssetPlayer"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kPlayer) } };
	REX::TTomlSetting<std::string> sAssetCombat{ "Assets"sv, "sAssetCombat"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kCombat) } };
	REX::TTomlSetting<std::string> sAssetPowerArmor{ "Assets"sv, "sAssetPowerArmor"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kPowerArmor) } };
	REX::TTomlSetting<std::string> sAssetIrradiated{ "Assets"sv, "sAssetIrradiated"sv, std::string{ Presence::DefaultAssetKey(Presence::Asset::kIrradiated) } };

	REX::TTomlSetting<std::string> sDetails{ "Format"sv, "sDetails"sv, std::string{ Config::kDefaultDetailsTemplate } };
	REX::TTomlSetting<std::string> sState{ "Format"sv, "sState"sv, std::string{ Config::kDefaultStateTemplate } };
	REX::TTomlSetting<std::string> sLargeText{ "Format"sv, "sLargeText"sv, std::string{ Config::kDefaultLargeTextTemplate } };
	REX::TTomlSetting<std::string> sSmallText{ "Format"sv, "sSmallText"sv, std::string{ Config::kDefaultSmallTextTemplate } };
	REX::TTomlSetting<std::string> sCombatSmallText{ "Format"sv, "sCombatSmallText"sv, std::string{ Config::kDefaultCombatSmallTextTemplate } };
}

namespace
{
	inline constexpr auto kCustomPath = "Data/F4SE/Plugins/Fallout4RichPresenceCustom.toml"sv;
	inline constexpr auto kCustomHeader = "# User overrides saved by Fallout4RichPresence.\n";

	[[nodiscard]] constexpr std::size_t AssetIndex(Presence::Asset a_asset) noexcept
	{
		switch (a_asset)
		{
			case Presence::Asset::kFallout4:
				return 0;
			case Presence::Asset::kMainMenu:
				return 1;
			case Presence::Asset::kLoading:
				return 2;
			case Presence::Asset::kCharacterCreation:
				return 3;
			case Presence::Asset::kPlayer:
				return 4;
			case Presence::Asset::kCombat:
				return 5;
			case Presence::Asset::kPowerArmor:
				return 6;
			case Presence::Asset::kIrradiated:
				return 7;
		}

		return 0;
	}

	std::atomic<std::shared_ptr<const Config::Snapshot>> g_current{
		std::make_shared<const Config::Snapshot>()
	};

	[[nodiscard]] std::string ResolveAssetKey(
		REX::TTomlSetting<std::string>& a_setting,
		Presence::Asset                 a_asset,
		std::string_view                a_name,
		std::string_view                a_previous,
		Config::Validation              a_validation)
	{
		auto value = a_setting.GetValue();
		if (Presence::IsValidAssetKey(value))
		{
			return value;
		}

		if (a_validation == Config::Validation::kQuiet)
		{
			return std::string{ a_previous };
		}

		// the installed preset is the user's chosen baseline, so prefer it over the compiled-in key
		value = a_setting.GetValueDefault();
		if (!Presence::IsValidAssetKey(value))
		{
			value = Presence::DefaultAssetKey(a_asset);
		}

		a_setting.SetValue(value);
		REX::WARN("Assets.{} must be up to 32 lowercase ASCII letters, digits, or underscores, or empty for no image; using \"{}\"", a_name, value);
		return value;
	}

	[[nodiscard]] Presence::FormatTemplate ResolveTemplate(
		REX::TTomlSetting<std::string>& a_setting,
		std::string_view                a_default,
		std::string_view                a_name,
		const Presence::FormatTemplate& a_previous,
		Config::Validation              a_validation)
	{
		auto result = Presence::FormatTemplate::Compile(a_setting.GetValue());
		if (result)
		{
			return std::move(*result);
		}

		// every prefix of "{quest}" is invalid, so a keystroke must not discard the last good template
		if (a_validation == Config::Validation::kQuiet)
		{
			return a_previous;
		}

		REX::WARN("Format.{} is invalid at byte {} ({}); using the installed preset", a_name, result.error().position, result.error().message);

		auto fallback = a_setting.GetValueDefault();
		auto compiled = Presence::FormatTemplate::Compile(fallback);
		if (!compiled)
		{
			fallback = std::string{ a_default };
			compiled = Presence::FormatTemplate::Compile(fallback);
		}

		a_setting.SetValue(fallback);
		if (compiled)
		{
			return std::move(*compiled);
		}

		REX::ERROR("Compiled-in default Format.{} is invalid; field disabled", a_name);
		return {};
	}

	void RemoveSetting(toml::value& a_output, std::string_view a_section, std::string_view a_key)
	{
		auto&      root = a_output.as_table();
		const auto section = root.find(std::string{ a_section });
		if (section == root.end() || !section->second.is_table())
		{
			return;
		}

		section->second.as_table().erase(std::string{ a_key });
		if (section->second.as_table().empty())
		{
			root.erase(section);
		}
	}

	template <class T>
	void WriteOverride(toml::value& a_output, REX::TTomlSetting<T>& a_setting, std::string_view a_section, std::string_view a_key)
	{
		if (a_setting.GetValue() != a_setting.GetValueDefault())
		{
			a_setting.Save(&a_output);
		}
		else
		{
			RemoveSetting(a_output, a_section, a_key);
		}
	}

	[[nodiscard]] bool MarkImplicitTables(toml::value& a_value)
	{
		for (auto& [key, value] : a_value.as_table())
		{
			(void)key;
			if (value.is_table())
			{
				if (!MarkImplicitTables(value))
				{
					continue;
				}
				value.as_table_fmt().fmt = toml::table_format::implicit;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	void LogSaveError(std::string_view a_reason) noexcept
	{
		try
		{
			REX::ERROR("Failed to save {}: {}", kCustomPath, a_reason);
		}
		catch (...)
		{}
	}
}

namespace Config
{
	std::string_view Snapshot::GetAssetKey(Presence::Asset a_asset) const noexcept
	{
		return assetKeys[AssetIndex(a_asset)];
	}

	void Load()
	{
		const auto store = REX::FTomlSettingStore::GetSingleton();
		store->Init(
			"Data/F4SE/Plugins/Fallout4RichPresence.toml",
			kCustomPath.data());
		store->Load();
		Rebuild();
	}

	void Rebuild(Validation a_validation)
	{
		if (iSamplingIntervalMs.GetValue() <= 0)
		{
			if (a_validation == Validation::kStrict)
			{
				REX::WARN("iSamplingIntervalMs must be positive; using {}", kDefaultSamplingInterval.count());
			}
			iSamplingIntervalMs.SetValue(static_cast<std::int32_t>(kDefaultSamplingInterval.count()));
		}

		const auto irradiatedPercent = std::clamp(
			iIrradiatedPercent.GetValue(),
			kMinimumIrradiatedPercent,
			kMaximumIrradiatedPercent);
		if (irradiatedPercent != iIrradiatedPercent.GetValue())
		{
			if (a_validation == Validation::kStrict)
			{
				REX::WARN("iIrradiatedPercent must be between {} and {}; using {}",
					kMinimumIrradiatedPercent,
					kMaximumIrradiatedPercent,
					irradiatedPercent);
			}
			iIrradiatedPercent.SetValue(irradiatedPercent);
		}

		const auto markerMaxDistance = std::clamp(
			iMarkerMaxDistance.GetValue(),
			kMinimumMarkerMaxDistance,
			kMaximumMarkerMaxDistance);
		if (markerMaxDistance != iMarkerMaxDistance.GetValue())
		{
			if (a_validation == Validation::kStrict)
			{
				REX::WARN("iMarkerMaxDistance must be between {} and {}; using {}",
					kMinimumMarkerMaxDistance,
					kMaximumMarkerMaxDistance,
					markerMaxDistance);
			}
			iMarkerMaxDistance.SetValue(markerMaxDistance);
		}

		const auto previous = Current();
		auto       snapshot = std::make_shared<Snapshot>();
		snapshot->samplingInterval = std::chrono::milliseconds{ iSamplingIntervalMs.GetValue() };
		snapshot->irradiatedPercent = iIrradiatedPercent.GetValue();
		snapshot->debugLogging = bDebugLogging.GetValue();
		snapshot->showPlayerName = bShowPlayerName.GetValue();
		snapshot->showQuest = bShowQuest.GetValue();
		snapshot->showLocation = bShowLocation.GetValue();
		snapshot->showExactLocation = bShowExactLocation.GetValue();
		snapshot->showCombatTarget = bShowCombatTarget.GetValue();
		snapshot->markerArtwork = bMarkerArtwork.GetValue();
		snapshot->stateBadge = bStateBadge.GetValue();
		snapshot->markerMaxDistance = iMarkerMaxDistance.GetValue();
		snapshot->assetKeys[AssetIndex(Presence::Asset::kFallout4)] = ResolveAssetKey(sAssetDefault, Presence::Asset::kFallout4, "sAssetDefault"sv, previous->GetAssetKey(Presence::Asset::kFallout4), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kMainMenu)] = ResolveAssetKey(sAssetMainMenu, Presence::Asset::kMainMenu, "sAssetMainMenu"sv, previous->GetAssetKey(Presence::Asset::kMainMenu), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kLoading)] = ResolveAssetKey(sAssetLoading, Presence::Asset::kLoading, "sAssetLoading"sv, previous->GetAssetKey(Presence::Asset::kLoading), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kCharacterCreation)] = ResolveAssetKey(sAssetCharacterCreation, Presence::Asset::kCharacterCreation, "sAssetCharacterCreation"sv, previous->GetAssetKey(Presence::Asset::kCharacterCreation), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kPlayer)] = ResolveAssetKey(sAssetPlayer, Presence::Asset::kPlayer, "sAssetPlayer"sv, previous->GetAssetKey(Presence::Asset::kPlayer), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kCombat)] = ResolveAssetKey(sAssetCombat, Presence::Asset::kCombat, "sAssetCombat"sv, previous->GetAssetKey(Presence::Asset::kCombat), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kPowerArmor)] = ResolveAssetKey(sAssetPowerArmor, Presence::Asset::kPowerArmor, "sAssetPowerArmor"sv, previous->GetAssetKey(Presence::Asset::kPowerArmor), a_validation);
		snapshot->assetKeys[AssetIndex(Presence::Asset::kIrradiated)] = ResolveAssetKey(sAssetIrradiated, Presence::Asset::kIrradiated, "sAssetIrradiated"sv, previous->GetAssetKey(Presence::Asset::kIrradiated), a_validation);
		snapshot->details = ResolveTemplate(sDetails, kDefaultDetailsTemplate, "sDetails"sv, previous->details, a_validation);
		snapshot->state = ResolveTemplate(sState, kDefaultStateTemplate, "sState"sv, previous->state, a_validation);
		snapshot->largeText = ResolveTemplate(sLargeText, kDefaultLargeTextTemplate, "sLargeText"sv, previous->largeText, a_validation);
		snapshot->smallText = ResolveTemplate(sSmallText, kDefaultSmallTextTemplate, "sSmallText"sv, previous->smallText, a_validation);
		snapshot->combatSmallText = ResolveTemplate(sCombatSmallText, kDefaultCombatSmallTextTemplate, "sCombatSmallText"sv, previous->combatSmallText, a_validation);
		g_current.store(std::move(snapshot), std::memory_order_release);
	}

	std::shared_ptr<const Snapshot> Current() noexcept
	{
		return g_current.load(std::memory_order_acquire);
	}

	std::string GetApplicationID()
	{
		return sApplicationID.GetValue();
	}

	bool SaveOverrides()
	{
		try
		{
			// toml11 default-constructs to an empty (non-table) value whose as_table() throws
			toml::value output{ toml::table{} };
			if (std::filesystem::exists(kCustomPath))
			{
				auto result = toml::try_parse(kCustomPath.data());
				if (!result.is_ok())
				{
					LogSaveError("the existing file is not valid TOML"sv);
					return false;
				}
				output = result.unwrap();
			}

			WriteOverride(output, iSamplingIntervalMs, "General"sv, "iSamplingIntervalMs"sv);
			WriteOverride(output, iIrradiatedPercent, "General"sv, "iIrradiatedPercent"sv);
			WriteOverride(output, bDebugLogging, "General"sv, "bDebugLogging"sv);
			WriteOverride(output, bShowPlayerName, "Privacy"sv, "bShowPlayerName"sv);
			WriteOverride(output, bShowQuest, "Privacy"sv, "bShowQuest"sv);
			WriteOverride(output, bShowLocation, "Privacy"sv, "bShowLocation"sv);
			WriteOverride(output, bShowExactLocation, "Privacy"sv, "bShowExactLocation"sv);
			WriteOverride(output, bShowCombatTarget, "Privacy"sv, "bShowCombatTarget"sv);
			WriteOverride(output, sApplicationID, "Discord"sv, "sApplicationID"sv);
			WriteOverride(output, bMarkerArtwork, "Assets"sv, "bMarkerArtwork"sv);
			WriteOverride(output, bStateBadge, "Assets"sv, "bStateBadge"sv);
			WriteOverride(output, iMarkerMaxDistance, "Assets"sv, "iMarkerMaxDistance"sv);
			WriteOverride(output, sAssetDefault, "Assets"sv, "sAssetDefault"sv);
			WriteOverride(output, sAssetMainMenu, "Assets"sv, "sAssetMainMenu"sv);
			WriteOverride(output, sAssetLoading, "Assets"sv, "sAssetLoading"sv);
			WriteOverride(output, sAssetCharacterCreation, "Assets"sv, "sAssetCharacterCreation"sv);
			WriteOverride(output, sAssetPlayer, "Assets"sv, "sAssetPlayer"sv);
			WriteOverride(output, sAssetCombat, "Assets"sv, "sAssetCombat"sv);
			WriteOverride(output, sAssetPowerArmor, "Assets"sv, "sAssetPowerArmor"sv);
			WriteOverride(output, sAssetIrradiated, "Assets"sv, "sAssetIrradiated"sv);
			WriteOverride(output, sDetails, "Format"sv, "sDetails"sv);
			WriteOverride(output, sState, "Format"sv, "sState"sv);
			WriteOverride(output, sLargeText, "Format"sv, "sLargeText"sv);
			WriteOverride(output, sSmallText, "Format"sv, "sSmallText"sv);
			WriteOverride(output, sCombatSmallText, "Format"sv, "sCombatSmallText"sv);

			(void)MarkImplicitTables(output);
			std::filesystem::create_directories(std::filesystem::path{ kCustomPath }.parent_path());
			std::ofstream file{ kCustomPath.data(), std::ios::trunc };
			if (!file)
			{
				LogSaveError("the file could not be opened for writing"sv);
				return false;
			}

			file << kCustomHeader;
			if (!output.as_table().empty())
			{
				file << '\n'
					 << toml::format(output);
			}
			file.flush();
			if (!file)
			{
				LogSaveError("the write did not complete"sv);
				return false;
			}
			return true;
		}
		catch (const std::exception& a_exception)
		{
			LogSaveError(a_exception.what());
		}
		catch (...)
		{
			LogSaveError("unknown failure"sv);
		}
		return false;
	}
}
