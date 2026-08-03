#include "pch.h"

#include "Config.h"
#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace Config
{
	REX::TTomlSetting<std::int32_t> iSamplingIntervalMs{
		"General"sv,
		"iSamplingIntervalMs"sv,
		static_cast<std::int32_t>(kDefaultSamplingInterval.count())
	};
	REX::TTomlSetting<bool> bDebugLogging{ "General"sv, "bDebugLogging"sv, false };

	// name is personal, quest and location are spoilers; the shipped preset opts into the latter two
	REX::TTomlSetting<bool>        bShowPlayerName{ "Privacy"sv, "bShowPlayerName"sv, false };
	REX::TTomlSetting<bool>        bShowQuest{ "Privacy"sv, "bShowQuest"sv, true };
	REX::TTomlSetting<bool>        bShowLocation{ "Privacy"sv, "bShowLocation"sv, true };
	REX::TTomlSetting<bool>        bShowExactLocation{ "Privacy"sv, "bShowExactLocation"sv, true };
	REX::TTomlSetting<std::string> sApplicationID{ "Discord"sv, "sApplicationID"sv, "1533687297684537374" };
}

namespace
{
	REX::TTomlSetting<std::string> sAssetDefault{ "Assets"sv, "sAssetDefault"sv, std::string{ Presence::kDefaultAssetKey } };
	REX::TTomlSetting<std::string> sAssetMainMenu{ "Assets"sv, "sAssetMainMenu"sv, std::string{ Presence::kDefaultAssetKey } };
	REX::TTomlSetting<std::string> sAssetLoading{ "Assets"sv, "sAssetLoading"sv, std::string{ Presence::kDefaultAssetKey } };
	REX::TTomlSetting<std::string> sAssetCharacterCreation{ "Assets"sv, "sAssetCharacterCreation"sv, std::string{ Presence::kDefaultAssetKey } };
	REX::TTomlSetting<std::string> sAssetPlayer{ "Assets"sv, "sAssetPlayer"sv, std::string{ Presence::kDefaultAssetKey } };
	REX::TTomlSetting<std::string> sAssetCombat{ "Assets"sv, "sAssetCombat"sv, std::string{ Presence::kDefaultAssetKey } };

	REX::TTomlSetting<std::string> sDetails{ "Format"sv, "sDetails"sv, std::string{ Config::kDefaultDetailsTemplate } };
	REX::TTomlSetting<std::string> sState{ "Format"sv, "sState"sv, std::string{ Config::kDefaultStateTemplate } };
	REX::TTomlSetting<std::string> sLargeText{ "Format"sv, "sLargeText"sv, std::string{ Config::kDefaultLargeTextTemplate } };
	REX::TTomlSetting<std::string> sSmallText{ "Format"sv, "sSmallText"sv, std::string{ Config::kDefaultSmallTextTemplate } };
	REX::TTomlSetting<std::string> sCombatSmallText{ "Format"sv, "sCombatSmallText"sv, std::string{ Config::kDefaultCombatSmallTextTemplate } };

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
		}

		return 0;
	}

	std::array<std::string, 6> assetKeys{
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kFallout4) },
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kMainMenu) },
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kLoading) },
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kCharacterCreation) },
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kPlayer) },
		std::string{ Presence::DefaultAssetKey(Presence::Asset::kCombat) }
	};

	Presence::FormatTemplate detailsTemplate;
	Presence::FormatTemplate stateTemplate;
	Presence::FormatTemplate largeTextTemplate;
	Presence::FormatTemplate smallTextTemplate;
	Presence::FormatTemplate combatSmallTextTemplate;

	void LoadAssetKey(REX::TTomlSetting<std::string>& a_setting, Presence::Asset a_asset, std::string_view a_name)
	{
		auto value = a_setting.GetValue();
		if (!Presence::IsValidAssetKey(value))
		{
			value = Presence::DefaultAssetKey(a_asset);
			a_setting.SetValue(value);
			REX::WARN("Assets.{} must be up to 32 lowercase ASCII letters, digits, or underscores, or empty for no image; using \"{}\"", a_name, value);
		}
		assetKeys[AssetIndex(a_asset)] = std::move(value);
	}

	[[nodiscard]] Presence::FormatTemplate CompileDefaultTemplate(std::string_view a_default, std::string_view a_name)
	{
		auto result = Presence::FormatTemplate::Compile(a_default);
		if (result)
		{
			return std::move(*result);
		}

		REX::ERROR("Compiled-in default Format.{} is invalid at byte {} ({}); field disabled", a_name, result.error().position, result.error().message);
		return {};
	}

	[[nodiscard]] Presence::FormatTemplate LoadTemplate(REX::TTomlSetting<std::string>& a_setting, std::string_view a_default, std::string_view a_name)
	{
		auto result = Presence::FormatTemplate::Compile(a_setting.GetValue());
		if (result)
		{
			return std::move(*result);
		}

		REX::WARN("Format.{} is invalid at byte {} ({}); using compiled-in default", a_name, result.error().position, result.error().message);
		a_setting.SetValue(std::string{ a_default });
		return CompileDefaultTemplate(a_default, a_name);
	}
}

namespace Config
{
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

		LoadAssetKey(sAssetDefault, Presence::Asset::kFallout4, "sAssetDefault"sv);
		LoadAssetKey(sAssetMainMenu, Presence::Asset::kMainMenu, "sAssetMainMenu"sv);
		LoadAssetKey(sAssetLoading, Presence::Asset::kLoading, "sAssetLoading"sv);
		LoadAssetKey(sAssetCharacterCreation, Presence::Asset::kCharacterCreation, "sAssetCharacterCreation"sv);
		LoadAssetKey(sAssetPlayer, Presence::Asset::kPlayer, "sAssetPlayer"sv);
		LoadAssetKey(sAssetCombat, Presence::Asset::kCombat, "sAssetCombat"sv);

		detailsTemplate = LoadTemplate(sDetails, kDefaultDetailsTemplate, "sDetails"sv);
		stateTemplate = LoadTemplate(sState, kDefaultStateTemplate, "sState"sv);
		largeTextTemplate = LoadTemplate(sLargeText, kDefaultLargeTextTemplate, "sLargeText"sv);
		smallTextTemplate = LoadTemplate(sSmallText, kDefaultSmallTextTemplate, "sSmallText"sv);
		combatSmallTextTemplate = LoadTemplate(sCombatSmallText, kDefaultCombatSmallTextTemplate, "sCombatSmallText"sv);
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

	bool ShowExactLocation() noexcept
	{
		return bShowExactLocation.GetValue();
	}

	std::string GetApplicationID()
	{
		return sApplicationID.GetValue();
	}

	std::string_view GetAssetKey(Presence::Asset a_asset) noexcept
	{
		return assetKeys[AssetIndex(a_asset)];
	}

	const Presence::FormatTemplate& GetDetailsTemplate() noexcept
	{
		return detailsTemplate;
	}

	const Presence::FormatTemplate& GetStateTemplate() noexcept
	{
		return stateTemplate;
	}

	const Presence::FormatTemplate& GetLargeTextTemplate() noexcept
	{
		return largeTextTemplate;
	}

	const Presence::FormatTemplate& GetSmallTextTemplate() noexcept
	{
		return smallTextTemplate;
	}

	const Presence::FormatTemplate& GetCombatSmallTextTemplate() noexcept
	{
		return combatSmallTextTemplate;
	}
}
