#include "Host/Client.h"
#include "Host/Navigation.h"

#include <DearModdingUI/IconGlyphs.h>
#include <DearModdingUI/UI.h>

#include "Config.h"
#include "Discord/Worker.h"
#include "Game/Tick.h"
#include "Logging.h"
#include "Presence/Activity.h"
#include "Presence/FormatTemplate.h"

#include <REX/TTomlSetting.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Host
{
	namespace
	{
		inline constexpr auto kClientID = "dearmodding.richpresence";
		inline constexpr auto kClientDisplayName = "Rich Presence";
		inline constexpr auto kFormatTokens = "Available tokens: {name} {level} {quest} {objective} {location} {worldspace} {state} {target} {activity}.";
		inline constexpr auto kAssetDescription = "Use 1-32 lowercase ASCII letters, digits, or underscores, or leave empty for no image.";

		const std::array kQuickLinks{
			dmui::Link{
				.label = "GitHub",
				.external = {
					.targetKind = DMUI_EXTERNAL_TARGET_URI,
					.target = "https://github.com/northaxosky/fallout4-rich-presence" },
				.note = "Open the Fallout 4 Rich Presence project in your browser.",
				.glyph = DearModdingUI::FindPhosphorSlugGlyphOrZero("github-logo"),
				.action = dmui::LinkAction::kOpenExternal }
		};

		constexpr std::array kFaqEntries{
			dmui::FaqEntry{
				"Why is one of my images blank?",
				"The image key has not been uploaded to the Discord application, or the key was typed incorrectly. Keys are 1-32 lowercase letters, digits, and underscores." },
			dmui::FaqEntry{
				"How do I hide my location or quest?",
				"Use the Privacy group on the Settings page, or install the Spoiler-free preset." },
			dmui::FaqEntry{
				"Which Fallout 4 runtimes are supported?",
				"One DLL supports 1.10.163, 1.10.984, 1.11.221, and 1.11.240." },
			dmui::FaqEntry{
				"Where are my settings stored?",
				"Data/F4SE/Plugins/Fallout4RichPresence.toml is the installed preset. Put personal overrides in Fallout4RichPresenceCustom.toml beside it so they survive reinstalling." }
		};

		enum class SettingSlot : std::size_t
		{
			kSamplingInterval,
			kIrradiatedPercent,
			kDebugLogging,
			kShowPlayerName,
			kShowQuest,
			kShowLocation,
			kShowExactLocation,
			kShowCombatTarget,
			kShowMenuActivity,
			kApplicationID,
			kMarkerArtwork,
			kStateBadge,
			kMarkerMaxDistance,
			kAssetDefault,
			kAssetMainMenu,
			kAssetLoading,
			kAssetCharacterCreation,
			kAssetPlayer,
			kAssetCombat,
			kAssetPowerArmor,
			kAssetIrradiated,
			kDetails,
			kState,
			kLargeText,
			kSmallText,
			kCombatSmallText,
			kLabelMainMenu,
			kLabelLoading,
			kLabelCharacterCreation,
			kLabelGameTitle,
			kLabelInGame,
			kLabelInCombat,
			kLabelInPowerArmor,
			kLabelIrradiated,
			kLabelLevel,
			kLabelBarter,
			kLabelBarterNamed,
			kLabelWorkbench,
			kLabelWorkbenchNamed,
			kLabelWorkshop,
			kLabelTerminal,
			kLabelLockpicking,
			kLabelSitWait,
			kLabelDialogue,
			kCount
		};

		dmui::Client g_client{
			kClientID,
			kClientDisplayName,
			dmui::Version{ PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR },
			kClientIcon,
			{},
			kClientOptions
		};
		std::array<dmui::SettingValue, static_cast<std::size_t>(SettingSlot::kCount)> g_savedValues{};
		Discord::Status                                                               g_status{};
		std::vector<Game::Conflict>                                                   g_conflicts;
		DMUI_Result                                                                   g_linkRowResult{ DMUI_RESULT_OK };
		DMUI_Result                                                                   g_faqResult{ DMUI_RESULT_OK };

		using GetHostAPIFn = const DMUI_HostAPI*(DMUI_CALL*)(std::uint32_t) noexcept;

		[[nodiscard]] constexpr std::size_t SlotIndex(SettingSlot a_slot) noexcept
		{
			return static_cast<std::size_t>(a_slot);
		}

		void AttachFeedback(dmui::SettingDescriptor& a_descriptor, const REX::ISetting& a_setting)
		{
			a_descriptor.resolveFeedback = [setting = &a_setting]() -> std::optional<dmui::FieldFeedback> {
				const auto feedback = Config::GetFeedback(*setting);
				if (!feedback)
					return std::nullopt;
				dmui::FieldFeedbackSeverity severity{};
				switch (feedback->severity)
				{
					case Config::FeedbackSeverity::kInfo:
						severity = dmui::FieldFeedbackSeverity::kInfo;
						break;
					case Config::FeedbackSeverity::kWarning:
						severity = dmui::FieldFeedbackSeverity::kWarning;
						break;
					case Config::FeedbackSeverity::kError:
						severity = dmui::FieldFeedbackSeverity::kError;
						break;
				}
				return dmui::FieldFeedback{ severity, feedback->message };
			};
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
			g_savedValues[SlotIndex(SettingSlot::kIrradiatedPercent)] =
				static_cast<std::int64_t>(Config::iIrradiatedPercent.GetValue());
			CaptureValue(SettingSlot::kDebugLogging, Config::bDebugLogging);
			CaptureValue(SettingSlot::kShowPlayerName, Config::bShowPlayerName);
			CaptureValue(SettingSlot::kShowQuest, Config::bShowQuest);
			CaptureValue(SettingSlot::kShowLocation, Config::bShowLocation);
			CaptureValue(SettingSlot::kShowExactLocation, Config::bShowExactLocation);
			CaptureValue(SettingSlot::kShowCombatTarget, Config::bShowCombatTarget);
			CaptureValue(SettingSlot::kShowMenuActivity, Config::bShowMenuActivity);
			CaptureValue(SettingSlot::kApplicationID, Config::sApplicationID);
			CaptureValue(SettingSlot::kMarkerArtwork, Config::bMarkerArtwork);
			CaptureValue(SettingSlot::kStateBadge, Config::bStateBadge);
			g_savedValues[SlotIndex(SettingSlot::kMarkerMaxDistance)] =
				static_cast<std::int64_t>(Config::iMarkerMaxDistance.GetValue());
			CaptureValue(SettingSlot::kAssetDefault, Config::sAssetDefault);
			CaptureValue(SettingSlot::kAssetMainMenu, Config::sAssetMainMenu);
			CaptureValue(SettingSlot::kAssetLoading, Config::sAssetLoading);
			CaptureValue(SettingSlot::kAssetCharacterCreation, Config::sAssetCharacterCreation);
			CaptureValue(SettingSlot::kAssetPlayer, Config::sAssetPlayer);
			CaptureValue(SettingSlot::kAssetCombat, Config::sAssetCombat);
			CaptureValue(SettingSlot::kAssetPowerArmor, Config::sAssetPowerArmor);
			CaptureValue(SettingSlot::kAssetIrradiated, Config::sAssetIrradiated);
			CaptureValue(SettingSlot::kDetails, Config::sDetails);
			CaptureValue(SettingSlot::kState, Config::sState);
			CaptureValue(SettingSlot::kLargeText, Config::sLargeText);
			CaptureValue(SettingSlot::kSmallText, Config::sSmallText);
			CaptureValue(SettingSlot::kCombatSmallText, Config::sCombatSmallText);
			CaptureValue(SettingSlot::kLabelMainMenu, Config::sLabelMainMenu);
			CaptureValue(SettingSlot::kLabelLoading, Config::sLabelLoading);
			CaptureValue(SettingSlot::kLabelCharacterCreation, Config::sLabelCharacterCreation);
			CaptureValue(SettingSlot::kLabelGameTitle, Config::sLabelGameTitle);
			CaptureValue(SettingSlot::kLabelInGame, Config::sLabelInGame);
			CaptureValue(SettingSlot::kLabelInCombat, Config::sLabelInCombat);
			CaptureValue(SettingSlot::kLabelInPowerArmor, Config::sLabelInPowerArmor);
			CaptureValue(SettingSlot::kLabelIrradiated, Config::sLabelIrradiated);
			CaptureValue(SettingSlot::kLabelLevel, Config::sLabelLevel);
			CaptureValue(SettingSlot::kLabelBarter, Config::sLabelBarter);
			CaptureValue(SettingSlot::kLabelBarterNamed, Config::sLabelBarterNamed);
			CaptureValue(SettingSlot::kLabelWorkbench, Config::sLabelWorkbench);
			CaptureValue(SettingSlot::kLabelWorkbenchNamed, Config::sLabelWorkbenchNamed);
			CaptureValue(SettingSlot::kLabelWorkshop, Config::sLabelWorkshop);
			CaptureValue(SettingSlot::kLabelTerminal, Config::sLabelTerminal);
			CaptureValue(SettingSlot::kLabelLockpicking, Config::sLabelLockpicking);
			CaptureValue(SettingSlot::kLabelSitWait, Config::sLabelSitWait);
			CaptureValue(SettingSlot::kLabelDialogue, Config::sLabelDialogue);
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
			AttachFeedback(descriptor, *setting);
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
			AttachFeedback(descriptor, *setting);
			return descriptor;
		}

		[[nodiscard]] dmui::SettingDescriptor MakeIrradiatedPercent()
		{
			auto* const             setting = &Config::iIrradiatedPercent;
			dmui::SettingDescriptor descriptor;
			descriptor.id = "iIrradiatedPercent";
			descriptor.label = "Irradiated threshold";
			descriptor.description = "Radiation percentage of the health pool at which the irradiated badge appears.";
			descriptor.control = dmui::SignedSettingControl{
				.range = dmui::NumericSettingRange<std::int64_t>{
					.minimum = Config::kMinimumIrradiatedPercent,
					.maximum = Config::kMaximumIrradiatedPercent },
				.format = "%lld%%",
				.dragSpeed = 1.0f
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
				       g_savedValues[SlotIndex(SettingSlot::kIrradiatedPercent)];
			};
			descriptor.isModified = [setting] {
				return setting->GetValue() != setting->GetValueDefault();
			};
			AttachFeedback(descriptor, *setting);
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
			AttachFeedback(descriptor, *setting);
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

		[[nodiscard]] bool DrawResolvedSectionHeader(
			const char* a_text,
			const char* a_explicitName,
			const char* a_secondaryMetadata = nullptr) noexcept
		{
			const auto glyph =
				g_client.ResolveIconGlyph(a_text, a_explicitName, a_secondaryMetadata);
			if (!glyph)
				return false;
			return g_client.DrawSectionHeader(
				a_text,
				*glyph ? *glyph : DearModdingUI::PhosphorGlyph::kQuestion);
		}

		[[nodiscard]] bool DrawPresenceField(const char* a_label, const std::string& a_value) noexcept
		{
			dmui::ui::TableNextRow();
			(void)dmui::ui::TableSetColumnIndex(0);
			dmui::ui::TextUnformatted(a_label);
			(void)dmui::ui::TableSetColumnIndex(1);
			if (!a_value.empty())
			{
				dmui::ui::TextUnformatted(a_value);
				return true;
			}

			return dmui::DrawStyledText(
				g_client,
				"—",
				{ .tone = dmui::TextTone::kMuted });
		}

		[[nodiscard]] bool DrawWelcome(const Discord::Status& a_status) noexcept
		{
			if (!dmui::DrawStyledText(
					g_client,
					"Fallout 4 Rich Presence",
					{ .fontRole = DMUI_FONT_ROLE_TITLE }))
				return false;
			dmui::ui::Spacing();
			if (!dmui::DrawStyledText(
					g_client,
					"Publishes your current Fallout 4 activity to Discord. Use the pages on the left to configure what is shared and how the card appears.",
					{ .fontRole = DMUI_FONT_ROLE_SUBTEXT,
						.wrapped = true }))
				return false;
			dmui::ui::Spacing();
			dmui::ui::Separator();
			dmui::ui::Spacing();

			static const auto runtime =
				REX::FModule::GetExecutingModule().GetFileVersion();
			dmui::ui::Text("Plugin version: %s", PLUGIN_VERSION);
			dmui::ui::Text(
				"Fallout 4 runtime: %u.%u.%u.%u",
				runtime.major(),
				runtime.minor(),
				runtime.patch(),
				runtime.build());
			dmui::ui::Text(
				"Map markers cached: %llu",
				static_cast<unsigned long long>(Game::Tick::GetMarkerCount()));

			const char*    health{};
			dmui::TextTone healthTone{ dmui::TextTone::kStatusWarning };
			if (a_status.state == Discord::ConnectionState::kDisabled)
			{
				health = "Health: Discord transport is disabled.";
				healthTone = dmui::TextTone::kStatusDisable;
			}
			else if (a_status.state == Discord::ConnectionState::kFailed ||
					 !g_conflicts.empty())
			{
				health =
					a_status.state == Discord::ConnectionState::kFailed ?
						"Health: Discord needs attention because the connection failed." :
						"Health: Discord needs attention because another presence plugin was detected.";
				healthTone = dmui::TextTone::kStatusError;
			}
			else if (a_status.state == Discord::ConnectionState::kConnected)
			{
				health = "Health: Discord is connected and no plugin conflicts were detected.";
				healthTone = dmui::TextTone::kStatusSuccess;
			}
			else
			{
				health = "Health: Waiting for Discord to connect.";
			}
			if (!dmui::DrawStyledText(
					g_client,
					health,
					{ .tone = healthTone }))
				return false;
			dmui::ui::Spacing();
			return true;
		}

		[[nodiscard]] bool DrawPresencePreview(const Presence::Activity& a_activity) noexcept
		{
			if (!DrawResolvedSectionHeader(
					"Live presence",
					"monitor",
					kClientDisplayName))
				return false;

			const auto labelWidth =
				dmui::ui::CalcTextSize("Small image text").x;
			if (dmui::ui::BeginTable(
					"LivePresence",
					2,
					dmui::ui::TableFlags::kRowBg |
						dmui::ui::TableFlags::kSizingStretchProp))
			{
				dmui::ui::TableSetupColumn(
					"Field",
					dmui::ui::TableColumnFlags::kWidthFixed,
					labelWidth);
				dmui::ui::TableSetupColumn(
					"Value",
					dmui::ui::TableColumnFlags::kWidthStretch);
				const auto fieldsDrawn =
					DrawPresenceField("Details", a_activity.details) &&
					DrawPresenceField("State", a_activity.state) &&
					DrawPresenceField("Large image key", a_activity.largeImage) &&
					DrawPresenceField("Large image text", a_activity.largeText) &&
					DrawPresenceField("Small image key", a_activity.smallImage) &&
					DrawPresenceField("Small image text", a_activity.smallText);

				if (a_activity.startTimestamp != 0)
				{
					const auto now =
						std::chrono::duration_cast<std::chrono::seconds>(
							std::chrono::system_clock::now().time_since_epoch())
							.count();
					const auto elapsed =
						now > a_activity.startTimestamp ?
							now - a_activity.startTimestamp :
							0;
					dmui::ui::TableNextRow();
					(void)dmui::ui::TableSetColumnIndex(0);
					dmui::ui::TextUnformatted("Elapsed");
					(void)dmui::ui::TableSetColumnIndex(1);
					dmui::ui::Text(
						"%lld:%02lld:%02lld",
						static_cast<long long>(elapsed / 3600),
						static_cast<long long>(elapsed / 60 % 60),
						static_cast<long long>(elapsed % 60));
				}
				dmui::ui::EndTable();
				if (!fieldsDrawn)
					return false;
			}

			dmui::ui::Spacing();
			if (!dmui::DrawStyledText(
					g_client,
					"An image key that has not been uploaded to the Discord application renders as a blank square.",
					{ .fontRole = DMUI_FONT_ROLE_SUBTEXT,
						.tone = dmui::TextTone::kMuted,
						.wrapped = true }))
				return false;
			dmui::ui::Spacing();
			return true;
		}

		[[nodiscard]] bool DrawConnection(const Discord::Status& a_status)
		{
			if (!DrawResolvedSectionHeader(
					"Connection",
					"gauge",
					"Discord"))
				return false;

			const auto stateTone = [&]() {
				switch (a_status.state)
				{
					case Discord::ConnectionState::kDisabled:
						return dmui::TextTone::kStatusDisable;
					case Discord::ConnectionState::kConnecting:
						return dmui::TextTone::kStatusWarning;
					case Discord::ConnectionState::kConnected:
						return dmui::TextTone::kStatusSuccess;
					case Discord::ConnectionState::kFailed:
						return dmui::TextTone::kStatusError;
				}
				return dmui::TextTone::kStatusInfo;
			}();
			const auto stateText =
				std::string{ "State: " } +
				ConnectionStateText(a_status.state);
			if (!dmui::DrawStyledText(
					g_client,
					stateText,
					{ .tone = stateTone }))
				return false;

			if (a_status.state == Discord::ConnectionState::kConnected)
			{
				dmui::ui::Text("Pipe: discord-ipc-%d", a_status.pipeIndex);
			}
			dmui::ui::Text(
				"Activities sent: %llu",
				static_cast<unsigned long long>(a_status.sentCount));
			if (!a_status.lastError.empty())
			{
				const auto error = std::string{ "Last error: " } + a_status.lastError;
				if (!dmui::DrawStyledText(
						g_client,
						error,
						{ .tone = dmui::TextTone::kStatusError }))
					return false;
			}

			for (const auto& conflict : g_conflicts)
			{
				const auto text =
					std::string{ "Conflict: " } +
					conflict.module +
					" (" +
					conflict.displayName +
					")";
				if (!dmui::DrawStyledText(
						g_client,
						text,
						{ .tone = dmui::TextTone::kStatusError }))
					return false;
			}
			if (!g_conflicts.empty())
			{
				if (!dmui::DrawStyledText(
						g_client,
						"Running two presence plugins can duplicate or flicker the Discord card.",
						{ .tone = dmui::TextTone::kStatusError,
							.wrapped = true }))
					return false;
			}
			dmui::ui::Spacing();
			return true;
		}

		[[nodiscard]] bool ReportHomeWidgetResult(
			bool         a_success,
			DMUI_Result& a_previous,
			const char*  a_message,
			bool         a_allowExternalOpenFailure = false)
		{
			if (a_success)
			{
				a_previous = DMUI_RESULT_OK;
				return true;
			}

			const auto result = g_client.LastResult();
			dmui::ui::TextDisabled("%s", a_message);
			if (result != a_previous)
			{
				a_previous = result;
				REX::WARN("{}: {}", a_message, DMUI_ResultToString(result));
			}
			if (!a_allowExternalOpenFailure || result != DMUI_RESULT_EXTERNAL_OPEN_FAILED)
				return false;

			// Consume each failed browser launch, including repeats; other API errors must reach the host.
			if (!g_client.SetStatus(DMUI_STATUS_SEVERITY_INFO, a_message))
			{
				REX::WARN("DearModdingUI status reporting failed: {}", DMUI_ResultToString(g_client.LastResult()));
				return false;
			}
			return true;
		}

		[[nodiscard]] bool DrawQuickLinks()
		{
			if (!DrawResolvedSectionHeader(
					"Quick Links",
					"link",
					"GitHub"))
				return false;
			if (!ReportHomeWidgetResult(
					g_client.DrawLinkRow("rich-presence-links", kQuickLinks),
					g_linkRowResult,
					"Project links failed. See Fallout4RichPresence.log for details.",
					true))
				return false;
			dmui::ui::Spacing();
			return true;
		}

		[[nodiscard]] bool DrawFaq()
		{
			if (!DrawResolvedSectionHeader(
					"FAQ",
					"question",
					"Help"))
				return false;
			return ReportHomeWidgetResult(
				g_client.DrawFaq("rich-presence-faq", kFaqEntries),
				g_faqResult,
				"FAQ unavailable. See Fallout4RichPresence.log for details.");
		}

		void DrawHome()
		{
			const auto status = Discord::Worker::GetStatus();
			const auto activity = Game::Tick::GetPublishedActivity();

			if (!DrawWelcome(status))
				return;
			if (!DrawPresencePreview(activity))
				return;
			if (!DrawConnection(status))
				return;
			if (!DrawQuickLinks())
				return;
			(void)DrawFaq();
		}

		template <class T>
		void RestoreDefault(REX::TTomlSetting<T>& a_setting)
		{
			a_setting.SetValue(a_setting.GetValueDefault());
		}

		void HandleSaveResult(Config::SaveResult a_result)
		{
			switch (a_result)
			{
				case Config::SaveResult::kSuccess:
					CaptureSavedValues();
					(void)g_client.SetStatus(
						DMUI_STATUS_SEVERITY_SUCCESS,
						"Settings saved.");
					break;
				case Config::SaveResult::kInvalidDraft:
					(void)g_client.SetStatus(
						DMUI_STATUS_SEVERITY_WARNING,
						"Fix invalid settings before saving.");
					break;
				case Config::SaveResult::kIOError:
					(void)g_client.SetStatus(
						DMUI_STATUS_SEVERITY_ERROR,
						"Settings could not be saved. See Fallout4RichPresence.log for details.");
					break;
			}
		}

		void ResetSettings()
		{
			RestoreDefault(Config::iSamplingIntervalMs);
			RestoreDefault(Config::iIrradiatedPercent);
			RestoreDefault(Config::bDebugLogging);
			RestoreDefault(Config::bShowPlayerName);
			RestoreDefault(Config::bShowQuest);
			RestoreDefault(Config::bShowLocation);
			RestoreDefault(Config::bShowExactLocation);
			RestoreDefault(Config::bShowCombatTarget);
			RestoreDefault(Config::bShowMenuActivity);
			RestoreDefault(Config::sApplicationID);
			RestoreDefault(Config::bMarkerArtwork);
			RestoreDefault(Config::bStateBadge);
			RestoreDefault(Config::iMarkerMaxDistance);
			RestoreDefault(Config::sAssetDefault);
			RestoreDefault(Config::sAssetMainMenu);
			RestoreDefault(Config::sAssetLoading);
			RestoreDefault(Config::sAssetCharacterCreation);
			RestoreDefault(Config::sAssetPlayer);
			RestoreDefault(Config::sAssetCombat);
			RestoreDefault(Config::sAssetPowerArmor);
			RestoreDefault(Config::sAssetIrradiated);
			RestoreDefault(Config::sDetails);
			RestoreDefault(Config::sState);
			RestoreDefault(Config::sLargeText);
			RestoreDefault(Config::sSmallText);
			RestoreDefault(Config::sCombatSmallText);
			RestoreDefault(Config::sLabelMainMenu);
			RestoreDefault(Config::sLabelLoading);
			RestoreDefault(Config::sLabelCharacterCreation);
			RestoreDefault(Config::sLabelGameTitle);
			RestoreDefault(Config::sLabelInGame);
			RestoreDefault(Config::sLabelInCombat);
			RestoreDefault(Config::sLabelInPowerArmor);
			RestoreDefault(Config::sLabelIrradiated);
			RestoreDefault(Config::sLabelLevel);
			RestoreDefault(Config::sLabelBarter);
			RestoreDefault(Config::sLabelBarterNamed);
			RestoreDefault(Config::sLabelWorkbench);
			RestoreDefault(Config::sLabelWorkbenchNamed);
			RestoreDefault(Config::sLabelWorkshop);
			RestoreDefault(Config::sLabelTerminal);
			RestoreDefault(Config::sLabelLockpicking);
			RestoreDefault(Config::sLabelSitWait);
			RestoreDefault(Config::sLabelDialogue);
			Config::Rebuild();
			Logging::Configure();
			HandleSaveResult(Config::SaveOverrides());
		}

		void ApplySettings()
		{
			HandleSaveResult(Config::SaveOverrides());
		}

		[[nodiscard]] dmui::SettingGroup MakeGeneralGroup()
		{
			const auto         checkbox = dmui::CheckboxSettingControl{};
			dmui::SettingGroup group;
			group.id = "general";
			group.label = "General";
			group.settings.push_back(MakeSamplingInterval());
			group.settings.push_back(MakeIrradiatedPercent());
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
			group.settings.push_back(MakeSetting(
				SettingSlot::kShowCombatTarget,
				"bShowCombatTarget",
				"Show combat target",
				"Makes {target} available to templates while in combat.",
				Config::bShowCombatTarget,
				checkbox));
			group.settings.push_back(MakeSetting(
				SettingSlot::kShowMenuActivity,
				"bShowMenuActivity",
				"Show menu activity",
				"Makes {activity} available and replaces details with the active menu activity.",
				Config::bShowMenuActivity,
				checkbox));
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
			group.settings.push_back(MakeSetting(
				SettingSlot::kStateBadge,
				"bStateBadge",
				"State badges",
				"Shows debounced power-armor and irradiated status badges during gameplay.",
				Config::bStateBadge,
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
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetPowerArmor,
				"sAssetPowerArmor",
				"Power armor image",
				kAssetDescription,
				Config::sAssetPowerArmor,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kAssetIrradiated,
				"sAssetIrradiated",
				"Irradiated image",
				kAssetDescription,
				Config::sAssetIrradiated,
				text,
				dmui::SettingApplyTiming::kImmediate));
			return group;
		}

		[[nodiscard]] dmui::SettingGroup MakeLabelsGroup()
		{
			const auto text = dmui::TextSettingControl{ .bufferCapacity = 128 };
			const auto templateText = dmui::TextSettingControl{
				.bufferCapacity = Presence::kFormatTemplateSourceLimit + 1
			};
			dmui::SettingGroup group;
			group.id = "labels";
			group.label = "Labels";
			group.expanded = false;
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelMainMenu,
				"sLabelMainMenu",
				"Main menu",
				"Text shown at the main menu.",
				Config::sLabelMainMenu,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelLoading,
				"sLabelLoading",
				"Loading",
				"Text shown while loading.",
				Config::sLabelLoading,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelCharacterCreation,
				"sLabelCharacterCreation",
				"Character creation",
				"Text shown during character creation.",
				Config::sLabelCharacterCreation,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelGameTitle,
				"sLabelGameTitle",
				"Game title",
				"Large-image tooltip for fixed game states.",
				Config::sLabelGameTitle,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelInGame,
				"sLabelInGame",
				"In game",
				"Label for normal gameplay.",
				Config::sLabelInGame,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelInCombat,
				"sLabelInCombat",
				"In combat",
				"Label and empty combat-tooltip fallback for combat.",
				Config::sLabelInCombat,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelInPowerArmor,
				"sLabelInPowerArmor",
				"In power armor",
				"Label for the power-armor state.",
				Config::sLabelInPowerArmor,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelIrradiated,
				"sLabelIrradiated",
				"Irradiated",
				"Label for the irradiated state.",
				Config::sLabelIrradiated,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelLevel,
				"sLabelLevel",
				"Level fallback",
				"Fallback when gameplay details and state are empty. Available token: {level}.",
				Config::sLabelLevel,
				templateText,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelBarter,
				"sLabelBarter",
				"Barter",
				"Barter activity when no vendor name is available.",
				Config::sLabelBarter,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelBarterNamed,
				"sLabelBarterNamed",
				"Named barter",
				"Barter activity with a vendor name. Available token: {name}.",
				Config::sLabelBarterNamed,
				templateText,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelWorkbench,
				"sLabelWorkbench",
				"Workbench",
				"Workbench activity when no furniture name is available.",
				Config::sLabelWorkbench,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelWorkbenchNamed,
				"sLabelWorkbenchNamed",
				"Named workbench",
				"Workbench activity with a furniture name. Available token: {name}.",
				Config::sLabelWorkbenchNamed,
				templateText,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelWorkshop,
				"sLabelWorkshop",
				"Workshop",
				"Workshop building activity.",
				Config::sLabelWorkshop,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelTerminal,
				"sLabelTerminal",
				"Terminal",
				"Terminal activity.",
				Config::sLabelTerminal,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelLockpicking,
				"sLabelLockpicking",
				"Lockpicking",
				"Lockpicking activity.",
				Config::sLabelLockpicking,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelSitWait,
				"sLabelSitWait",
				"Sit/wait",
				"Sit/wait activity.",
				Config::sLabelSitWait,
				text,
				dmui::SettingApplyTiming::kImmediate));
			group.settings.push_back(MakeSetting(
				SettingSlot::kLabelDialogue,
				"sLabelDialogue",
				"Dialogue",
				"Dialogue activity.",
				Config::sLabelDialogue,
				text,
				dmui::SettingApplyTiming::kImmediate));
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
				[] { dmui::ui::TextUnformatted(ConnectionStateText(g_status.state)); }));
			group.settings.push_back(MakeReadOnly(
				"pipeIndex",
				"Pipe",
				[] { dmui::ui::Text("discord-ipc-%d", g_status.pipeIndex); },
				[] { return g_status.state == Discord::ConnectionState::kConnected; }));
			group.settings.push_back(MakeReadOnly(
				"sentCount",
				"Activities sent",
				[] { dmui::ui::Text("%llu", static_cast<unsigned long long>(g_status.sentCount)); }));
			group.settings.push_back(MakeReadOnly(
				"lastError",
				"Last error",
				[] { dmui::ui::TextUnformatted(g_status.lastError); },
				[] { return !g_status.lastError.empty(); }));
			group.settings.push_back(MakeReadOnly(
				"pluginConflict",
				"Plugin conflict",
				[] {
					for (const auto& conflict : g_conflicts)
					{
						dmui::ui::Text("%s (%s)", conflict.displayName.c_str(), conflict.module.c_str());
					}
				},
				[] { return !g_conflicts.empty(); }));
			return group;
		}

		[[nodiscard]] dmui::SettingsPage MakeSettingsPage()
		{
			dmui::SettingsPage page;
			page.groups.push_back(MakeStatusGroup());
			page.groups.push_back(MakeGeneralGroup());
			page.groups.push_back(MakePrivacyGroup());
			page.groups.push_back(MakeFormatGroup());
			page.groups.push_back(MakeLabelsGroup());
			page.groups.push_back(MakeAssetsGroup());
			page.groups.push_back(MakeDiscordGroup());
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

	void SetConflicts(std::vector<Game::Conflict> a_conflicts) noexcept
	{
		g_conflicts = std::move(a_conflicts);
	}

	void Connect() noexcept
	{
		try
		{
			const auto getHostAPI =
				dmui::detail::ResolveHostSymbol<GetHostAPIFn>("DMUI_GetAPI");
			if (getHostAPI)
			{
				const auto hostAPI = getHostAPI(DMUI_HOST_ABI_CURRENT);
				if (hostAPI && !HasFieldFeedbackAPI(hostAPI))
				{
					REX::ERROR(
						"DearModdingUI field feedback is unavailable; a host containing the d034b47 field-feedback API is required, so no in-game pages were registered");
					return;
				}
			}

			if (!g_client.Connect())
			{
				if (!g_client.HostPresent())
				{
					REX::INFO("No DearModdingUI host is loaded; no in-game settings page this session");
				}
				else
				{
					REX::ERROR(
						"DearModdingUI registration failed: {}; a matching host and client API build is required",
						DMUI_ResultToString(g_client.LastResult()));
				}
				return;
			}

			const auto generalIcon = g_client.ResolveIconGlyph(
				kGeneralCategory.displayName,
				kGeneralCategory.iconName,
				kClientDisplayName);
			if (!generalIcon)
			{
				REX::ERROR(
					"DearModdingUI icon resolution is unavailable: {}; host 0.1.2 or newer is required",
					DMUI_ResultToString(g_client.LastResult()));
				return;
			}

			if (!g_client.AddCategory(kGeneralCategory))
			{
				REX::ERROR(
					"DearModdingUI category registration failed: {}",
					DMUI_ResultToString(g_client.LastResult()));
				return;
			}

			CaptureSavedValues();
			const auto homePage = g_client.AddPage(kHomePage, &DrawHome);
			if (!homePage)
			{
				REX::ERROR(
					"DearModdingUI home-page registration failed: {}",
					DMUI_ResultToString(g_client.LastResult()));
				return;
			}

			const auto settingsPage = g_client.AddSettingsPage(kSettingsPage, MakeSettingsPage());
			if (!settingsPage)
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
