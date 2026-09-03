#include "Host/Client.h"

#include <DearModdingUI/Client.h>

#include "Config.h"
#include "Discord/Worker.h"
#include "Logging.h"
#include "Presence/Activity.h"
#include "Presence/FormatTemplate.h"

#include <REX/TTomlSetting.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace Host
{
	namespace
	{
		inline constexpr auto kClientID = "dearmodding.richpresence";
		inline constexpr auto kClientDisplayName = "Rich Presence";
		inline constexpr auto kClientIcon = "gauge";
		inline constexpr auto kFormatTokens = "Available tokens: {name} {level} {quest} {objective} {location} {worldspace} {state}.";
		inline constexpr auto kAssetDescription = "Use 1-32 lowercase ASCII letters, digits, or underscores, or leave empty for no image.";

		enum class SettingSlot : std::size_t
		{
			kSamplingInterval,
			kDebugLogging,
			kShowPlayerName,
			kShowQuest,
			kShowLocation,
			kShowExactLocation,
			kApplicationID,
			kMarkerArtwork,
			kMarkerMaxDistance,
			kAssetDefault,
			kAssetMainMenu,
			kAssetLoading,
			kAssetCharacterCreation,
			kAssetPlayer,
			kAssetCombat,
			kDetails,
			kState,
			kLargeText,
			kSmallText,
			kCombatSmallText,
			kCount
		};

		dmui::Client g_client{
			kClientID,
			kClientDisplayName,
			dmui::Version{ PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR },
			dmui::kForwardingClient,
			kClientIcon
		};
		std::array<dmui::SettingValue, static_cast<std::size_t>(SettingSlot::kCount)> g_savedValues{};
		Discord::Status                                                               g_status{};

		[[nodiscard]] constexpr std::size_t SlotIndex(SettingSlot a_slot) noexcept
		{
			return static_cast<std::size_t>(a_slot);
		}

		template <dmui::SettingValueAlternative T>
		void CaptureValue(SettingSlot a_slot, const REX::TTomlSetting<T>& a_setting)
		{
			g_savedValues[SlotIndex(a_slot)] = a_setting.GetValue();
		}

		void CaptureSavedValues()
		{
			g_savedValues[SlotIndex(SettingSlot::kSamplingInterval)] =
				static_cast<std::int64_t>(Config::iSamplingIntervalMs.GetValue());
			CaptureValue(SettingSlot::kDebugLogging, Config::bDebugLogging);
			CaptureValue(SettingSlot::kShowPlayerName, Config::bShowPlayerName);
			CaptureValue(SettingSlot::kShowQuest, Config::bShowQuest);
			CaptureValue(SettingSlot::kShowLocation, Config::bShowLocation);
			CaptureValue(SettingSlot::kShowExactLocation, Config::bShowExactLocation);
			CaptureValue(SettingSlot::kApplicationID, Config::sApplicationID);
			CaptureValue(SettingSlot::kMarkerArtwork, Config::bMarkerArtwork);
			g_savedValues[SlotIndex(SettingSlot::kMarkerMaxDistance)] =
				static_cast<std::int64_t>(Config::iMarkerMaxDistance.GetValue());
			CaptureValue(SettingSlot::kAssetDefault, Config::sAssetDefault);
			CaptureValue(SettingSlot::kAssetMainMenu, Config::sAssetMainMenu);
			CaptureValue(SettingSlot::kAssetLoading, Config::sAssetLoading);
			CaptureValue(SettingSlot::kAssetCharacterCreation, Config::sAssetCharacterCreation);
			CaptureValue(SettingSlot::kAssetPlayer, Config::sAssetPlayer);
			CaptureValue(SettingSlot::kAssetCombat, Config::sAssetCombat);
			CaptureValue(SettingSlot::kDetails, Config::sDetails);
			CaptureValue(SettingSlot::kState, Config::sState);
			CaptureValue(SettingSlot::kLargeText, Config::sLargeText);
			CaptureValue(SettingSlot::kSmallText, Config::sSmallText);
			CaptureValue(SettingSlot::kCombatSmallText, Config::sCombatSmallText);
		}

		template <dmui::SettingValueAlternative T>
		[[nodiscard]] dmui::SettingDescriptor MakeSetting(
			SettingSlot              a_slot,
			std::string_view         a_id,
			std::string_view         a_label,
			std::string_view         a_description,
			REX::TTomlSetting<T>&    a_setting,
			dmui::SettingControl     a_control,
			dmui::SettingApplyTiming a_applyTiming = dmui::SettingApplyTiming::kImmediate,
			std::function<void()>    a_afterSet = {})
		{
			auto* const             setting = &a_setting;
			dmui::SettingDescriptor descriptor;
			descriptor.id = a_id;
			descriptor.label = a_label;
			descriptor.description = a_description;
			descriptor.control = std::move(a_control);
			descriptor.defaultValue = setting->GetValueDefault();
			descriptor.binding = dmui::BindSetting(
				[setting] { return setting->GetValue(); },
				[setting, afterSet = std::move(a_afterSet)](T a_value) {
					setting->SetValue(std::move(a_value));
					Config::Rebuild(Config::Validation::kQuiet);
					if (afterSet)
					{
						afterSet();
					}
					return setting->GetValue();
				});
			descriptor.applyTiming = a_applyTiming;
			descriptor.isDirty = [setting, a_slot] {
				return dmui::SettingValue{ setting->GetValue() } != g_savedValues[SlotIndex(a_slot)];
			};
			descriptor.isModified = [setting] {
				return setting->GetValue() != setting->GetValueDefault();
			};
			return descriptor;
		}

		[[nodiscard]] dmui::SettingDescriptor MakeSamplingInterval()
		{
			auto* const             setting = &Config::iSamplingIntervalMs;
			dmui::SettingDescriptor descriptor;
			descriptor.id = "iSamplingIntervalMs";
			descriptor.label = "Sampling interval";
			descriptor.description = "Milliseconds between game-state samples.";
			descriptor.control = dmui::SignedSettingControl{
				.range = dmui::NumericSettingRange<std::int64_t>{
					.minimum = 100,
					.maximum = 5000 },
				.format = "%lld ms",
				.dragSpeed = 10.0f
			};
			descriptor.defaultValue = static_cast<std::int64_t>(setting->GetValueDefault());
			descriptor.binding = dmui::BindSetting(
				[setting] { return static_cast<std::int64_t>(setting->GetValue()); },
				[setting](std::int64_t a_value) {
					const auto value = std::clamp(
						a_value,
						static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
						static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()));
					setting->SetValue(static_cast<std::int32_t>(value));
					Config::Rebuild(Config::Validation::kQuiet);
					return static_cast<std::int64_t>(setting->GetValue());
				});
			descriptor.applyTiming = dmui::SettingApplyTiming::kImmediate;
			descriptor.isDirty = [setting] {
				return dmui::SettingValue{ static_cast<std::int64_t>(setting->GetValue()) } !=
				       g_savedValues[SlotIndex(SettingSlot::kSamplingInterval)];
			};
			descriptor.isModified = [setting] {
				return setting->GetValue() != setting->GetValueDefault();
			};
			return descriptor;
		}

		[[nodiscard]] dmui::SettingDescriptor MakeMarkerMaxDistance()
		{
			auto* const             setting = &Config::iMarkerMaxDistance;
			dmui::SettingDescriptor descriptor;
			descriptor.id = "iMarkerMaxDistance";
			descriptor.label = "Marker maximum distance";
			descriptor.description = "Maximum game-unit distance for nearest discovered marker artwork and interior location fallback.";
			descriptor.control = dmui::SignedSettingControl{
				.range = dmui::NumericSettingRange<std::int64_t>{
					.minimum = Config::kMinimumMarkerMaxDistance,
					.maximum = Config::kMaximumMarkerMaxDistance },
				.format = "%lld units",
				.dragSpeed = 256.0f
			};
			descriptor.defaultValue = static_cast<std::int64_t>(setting->GetValueDefault());
			descriptor.binding = dmui::BindSetting(
				[setting] { return static_cast<std::int64_t>(setting->GetValue()); },
				[setting](std::int64_t a_value) {
					const auto value = std::clamp(
						a_value,
						static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
						static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()));
					setting->SetValue(static_cast<std::int32_t>(value));
					Config::Rebuild(Config::Validation::kQuiet);
					return static_cast<std::int64_t>(setting->GetValue());
				});
			descriptor.applyTiming = dmui::SettingApplyTiming::kImmediate;
			descriptor.isDirty = [setting] {
				return dmui::SettingValue{ static_cast<std::int64_t>(setting->GetValue()) } !=
				       g_savedValues[SlotIndex(SettingSlot::kMarkerMaxDistance)];
			};
			descriptor.isModified = [setting] {
				return setting->GetValue() != setting->GetValueDefault();
			};
			return descriptor;
		}

		[[nodiscard]] dmui::SettingDescriptor MakeReadOnly(
			std::string_view      a_id,
			std::string_view      a_label,
			std::function<void()> a_draw,
			std::function<bool()> a_isVisible = {})
		{
			dmui::SettingDescriptor descriptor;
			descriptor.id = a_id;
			descriptor.label = a_label;
			descriptor.control = dmui::ReadOnlySettingControl{ .draw = std::move(a_draw) };
			descriptor.isVisible = std::move(a_isVisible);
			descriptor.showReset = false;
			return descriptor;
		}

		[[nodiscard]] const char* ConnectionStateText(Discord::ConnectionState a_state) noexcept
		{
			switch (a_state)
			{
				case Discord::ConnectionState::kDisabled:
					return "Disabled";
				case Discord::ConnectionState::kConnecting:
					return "Connecting";
				case Discord::ConnectionState::kConnected:
					return "Connected";
				case Discord::ConnectionState::kFailed:
					return "Failed";
			}
			return "Unknown";
		}

		template <class T>
		void RestoreDefault(REX::TTomlSetting<T>& a_setting)
		{
			a_setting.SetValue(a_setting.GetValueDefault());
		}

		void ResetSettings()
		{
			RestoreDefault(Config::iSamplingIntervalMs);
			RestoreDefault(Config::bDebugLogging);
			RestoreDefault(Config::bShowPlayerName);
			RestoreDefault(Config::bShowQuest);
			RestoreDefault(Config::bShowLocation);
			RestoreDefault(Config::bShowExactLocation);
			RestoreDefault(Config::sApplicationID);
			RestoreDefault(Config::bMarkerArtwork);
			RestoreDefault(Config::iMarkerMaxDistance);
			RestoreDefault(Config::sAssetDefault);
			RestoreDefault(Config::sAssetMainMenu);
			RestoreDefault(Config::sAssetLoading);
			RestoreDefault(Config::sAssetCharacterCreation);
			RestoreDefault(Config::sAssetPlayer);
			RestoreDefault(Config::sAssetCombat);
			RestoreDefault(Config::sDetails);
			RestoreDefault(Config::sState);
			RestoreDefault(Config::sLargeText);
			RestoreDefault(Config::sSmallText);
			RestoreDefault(Config::sCombatSmallText);
			Config::Rebuild();
			Logging::Configure();
			if (Config::SaveOverrides())
			{
				CaptureSavedValues();
			}
		}

		void ApplySettings()
		{
			// the quiet live path leaves invalid text in place; correct it before it reaches disk
			Config::Rebuild(Config::Validation::kStrict);
			Logging::Configure();
			if (Config::SaveOverrides())
			{
				CaptureSavedValues();
			}
		}

		[[nodiscard]] dmui::SettingGroup MakeGeneralGroup()
		{
			const auto         checkbox = dmui::CheckboxSettingControl{};
			dmui::SettingGroup group;
			group.id = "general";
			group.label = "General";
			group.settings.push_back(MakeSamplingInterval());
			group.settings.push_back(MakeSetting(
				SettingSlot::kDebugLogging,
				"bDebugLogging",
				"Debug logging",
				"Enables diagnostic logging.",
				Config::bDebugLogging,
				checkbox,
				dmui::SettingApplyTiming::kImmediate,
				[] { Logging::Configure(); }));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakePrivacyGroup()
		{
			const auto         checkbox = dmui::CheckboxSettingControl{};
			dmui::SettingGroup group;
			group.id = "privacy";
			group.label = "Privacy";
			group.settings.push_back(MakeSetting(
				SettingSlot::kShowPlayerName,
				"bShowPlayerName",
				"Show player name",
				"Makes {name} available to templates.",
				Config::bShowPlayerName,
				checkbox));
			group.settings.push_back(MakeSetting(
				SettingSlot::kShowQuest,
				"bShowQuest",
				"Show quest",
				"Makes {quest} and {objective} available to templates.",
				Config::bShowQuest,
				checkbox));
			group.settings.push_back(MakeSetting(
				SettingSlot::kShowLocation,
				"bShowLocation",
				"Show location",
				"Makes {worldspace} available and permits location data.",
				Config::bShowLocation,
				checkbox));
			auto exactLocation = MakeSetting(
				SettingSlot::kShowExactLocation,
				"bShowExactLocation",
				"Show exact location",
				"Makes {location} available when location data is permitted.",
				Config::bShowExactLocation,
				checkbox);
			exactLocation.isEnabled = [] {
				return Config::bShowLocation.GetValue();
			};
			group.settings.push_back(std::move(exactLocation));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakeFormatGroup()
		{
			const auto control = dmui::TextSettingControl{
				.bufferCapacity = Presence::kFormatTemplateSourceLimit + 1
			};
			dmui::SettingGroup group;
			group.id = "format";
			group.label = "Format";
			group.settings.push_back(MakeSetting(
				SettingSlot::kDetails,
				"sDetails",
				"Details",
				kFormatTokens,
				Config::sDetails,
				control));
			group.settings.push_back(MakeSetting(
				SettingSlot::kState,
				"sState",
				"State",
				kFormatTokens,
				Config::sState,
				control));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLargeText,
				"sLargeText",
				"Large image tooltip",
				kFormatTokens,
				Config::sLargeText,
				control));
			group.settings.push_back(MakeSetting(
				SettingSlot::kSmallText,
				"sSmallText",
				"Player image tooltip",
				kFormatTokens,
				Config::sSmallText,
				control));
			group.settings.push_back(MakeSetting(
				SettingSlot::kCombatSmallText,
				"sCombatSmallText",
				"Combat image tooltip",
				kFormatTokens,
				Config::sCombatSmallText,
				control));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakeAssetsGroup()
		{
			const auto checkbox = dmui::CheckboxSettingControl{};
			const auto text = dmui::TextSettingControl{
				.bufferCapacity = Presence::kActivityAssetKeyLimit + 1
			};
			dmui::SettingGroup group;
			group.id = "assets";
			group.label = "Assets";
			group.settings.push_back(MakeSetting(
				SettingSlot::kMarkerArtwork,
				"bMarkerArtwork",
				"Marker artwork",
				"Uses the nearest discovered map marker as the gameplay image when location sharing is enabled.",
				Config::bMarkerArtwork,
				checkbox,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeMarkerMaxDistance());
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetDefault,
				"sAssetDefault",
				"Gameplay image",
				kAssetDescription,
				Config::sAssetDefault,
				text));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetMainMenu,
				"sAssetMainMenu",
				"Main menu image",
				kAssetDescription,
				Config::sAssetMainMenu,
				text));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetLoading,
				"sAssetLoading",
				"Loading image",
				kAssetDescription,
				Config::sAssetLoading,
				text));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetCharacterCreation,
				"sAssetCharacterCreation",
				"Character creation image",
				kAssetDescription,
				Config::sAssetCharacterCreation,
				text));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetPlayer,
				"sAssetPlayer",
				"Player image",
				kAssetDescription,
				Config::sAssetPlayer,
				text));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetCombat,
				"sAssetCombat",
				"Combat image",
				kAssetDescription,
				Config::sAssetCombat,
				text));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakeDiscordGroup()
		{
			dmui::SettingGroup group;
			group.id = "discord";
			group.label = "Discord";
			group.settings.push_back(MakeSetting(
				SettingSlot::kApplicationID,
				"sApplicationID",
				"Application ID",
				"Discord application ID. Restart Fallout 4 to use a changed value.",
				Config::sApplicationID,
				dmui::TextSettingControl{ .bufferCapacity = 32 },
				dmui::SettingApplyTiming::kNextLaunch));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakeStatusGroup()
		{
			dmui::SettingGroup group;
			group.id = "status";
			group.label = "Status";
			group.settings.push_back(MakeReadOnly(
				"connectionState",
				"Connection",
				[] { ImGui::TextUnformatted(ConnectionStateText(g_status.state)); }));
			group.settings.push_back(MakeReadOnly(
				"pipeIndex",
				"Pipe",
				[] { ImGui::Text("discord-ipc-%d", g_status.pipeIndex); },
				[] { return g_status.state == Discord::ConnectionState::kConnected; }));
			group.settings.push_back(MakeReadOnly(
				"sentCount",
				"Activities sent",
				[] { ImGui::Text("%llu", static_cast<unsigned long long>(g_status.sentCount)); }));
			group.settings.push_back(MakeReadOnly(
				"lastError",
				"Last error",
				[] { ImGui::TextUnformatted(g_status.lastError.c_str()); },
				[] { return !g_status.lastError.empty(); }));
			return group;
		}

		[[nodiscard]] dmui::SettingsPage MakeSettingsPage()
		{
			dmui::SettingsPage page;
			page.groups.push_back(MakeGeneralGroup());
			page.groups.push_back(MakePrivacyGroup());
			page.groups.push_back(MakeFormatGroup());
			page.groups.push_back(MakeAssetsGroup());
			page.groups.push_back(MakeDiscordGroup());
			page.groups.push_back(MakeStatusGroup());
			page.actions = dmui::SettingsPageActionCallbacks{
				.showReset = true,
				.reset = &ResetSettings,
				.apply = &ApplySettings
			};
			page.actionTooltips = dmui::SettingsPageActionTooltips{
				.reset = "Restore every setting to the installed preset and save it.",
				.apply = [](std::size_t a_pending) {
					return "Save " + std::to_string(a_pending) +
				           (a_pending == 1 ? " change" : " changes") +
				           " to Fallout4RichPresenceCustom.toml.";
				}
			};
			page.notes = {
				dmui::SettingsPageNote{
					.text = "Changes are written to Fallout4RichPresenceCustom.toml so they survive reinstalling the mod.",
					.muted = true }
			};
			page.prepare = [] {
				g_status = Discord::Worker::GetStatus();
			};
			return page;
		}
	}

	void Connect() noexcept
	{
		try
		{
			if (!g_client.Connect())
			{
				if (!g_client.HostPresent())
				{
					REX::INFO("No DearModdingUI host is loaded; no in-game settings page this session");
				}
				else
				{
					REX::ERROR(
						"DearModdingUI registration failed: {}",
						DMUI_ResultToString(g_client.LastResult()));
				}
				return;
			}

			if (!ImGui::IsForwardVersionCompatible())
			{
				REX::WARN("The DearModdingUI host uses an incompatible ImGui forwarding API");
				return;
			}

			CaptureSavedValues();
			auto page = g_client.AddSettingsPage(
				"settings",
				"Settings",
				kClientDisplayName,
				MakeSettingsPage(),
				"Configure Discord Rich Presence and inspect its connection.",
				10);
			if (!page)
			{
				REX::ERROR(
					"DearModdingUI settings-page registration failed: {}",
					DMUI_ResultToString(g_client.LastResult()));
				return;
			}

			REX::INFO("Registered as '{}' with the DearModdingUI host", kClientID);
		}
		catch (const std::exception& a_exception)
		{
			try
			{
				REX::ERROR("DearModdingUI setup failed: {}", a_exception.what());
			}
			catch (...)
			{}
		}
		catch (...)
		{
			try
			{
				REX::ERROR("DearModdingUI setup failed");
			}
			catch (...)
			{}
		}
	}
}
