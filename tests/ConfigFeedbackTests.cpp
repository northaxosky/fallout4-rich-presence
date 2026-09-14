#include "Config.h"
#include "Presence/AssetKeys.h"

#include <DearModdingUI/API.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace
{
	void Require(bool a_condition, std::string_view a_message)
	{
		if (!a_condition)
			throw std::runtime_error(std::string{ a_message });
	}

	struct TestDirectory
	{
		std::filesystem::path original{ std::filesystem::current_path() };
		std::filesystem::path temporary{ std::filesystem::temp_directory_path() /
										 ("RichPresenceFeedback-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())) };

		TestDirectory()
		{
			std::filesystem::create_directories(temporary / "Data/F4SE/Plugins");
			std::filesystem::current_path(temporary);
		}
		~TestDirectory()
		{
			std::filesystem::current_path(original);
			std::error_code error;
			std::filesystem::remove_all(temporary, error);
			if (error)
				std::cerr << "Test cleanup failed: " << error.message() << '\n';
		}
	};

	void Write(std::string_view a_path, std::string_view a_text)
	{
		std::ofstream file{ a_path.data(), std::ios::binary | std::ios::trunc };
		file << a_text;
		file.close();
		Require(!file.fail(), "fixture write failed");
	}

	std::string Read(std::string_view a_path)
	{
		std::ifstream file{ a_path.data(), std::ios::binary };
		Require(file.is_open(), "fixture read failed");
		return { std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
	}

	void ExpectFeedback(const REX::ISetting& a_setting, Config::FeedbackSeverity a_severity, bool a_blocksSave)
	{
		const auto feedback = Config::GetFeedback(a_setting);
		Require(feedback && feedback->severity == a_severity && feedback->blocksSave == a_blocksSave &&
					!feedback->message.empty(),
			"missing or incorrect feedback");
	}
}

int main()
try
{
	TestDirectory  directory;
	constexpr auto base = "Data/F4SE/Plugins/Fallout4RichPresence.toml";
	constexpr auto custom = "Data/F4SE/Plugins/Fallout4RichPresenceCustom.toml";
	constexpr auto startupID = "12345678901234567";
	Write(base, "[Discord]\nsApplicationID = \"12345678901234567\"\n");
	Write(custom, "[Unknown]\nkeep = \"yes\"\n");
	std::ostringstream log;
	spdlog::set_default_logger(std::make_shared<spdlog::logger>(
		"feedback", std::make_shared<spdlog::sinks::ostream_sink_mt>(log)));
	Config::Load();
	Require(!Config::HasBlockingFeedback(), "valid startup has blocking feedback");
	const auto rebuild = [] { Config::Rebuild(Config::Validation::kQuiet); };
	using Severity = Config::FeedbackSeverity;
	using Result = Config::SaveResult;

	for (const auto id : { startupID, "12345678901234567890" })
		Require(Config::IsPlausibleApplicationID(id), "valid application ID rejected");
	for (const auto id : { "1234567890123456", "123456789012345678901", "1234567890123456x" })
	{
		Config::sApplicationID.SetValue(id);
		rebuild();
		ExpectFeedback(Config::sApplicationID, Severity::kError, true);
	}
	Config::sApplicationID.SetValue("12345678901234567890");
	rebuild();
	ExpectFeedback(Config::sApplicationID, Severity::kInfo, false);
	Require(Config::SaveOverrides() == Result::kSuccess, "restart notice blocks save");
	ExpectFeedback(Config::sApplicationID, Severity::kInfo, false);
	Config::sApplicationID.SetValue(startupID);
	rebuild();
	Require(!Config::GetFeedback(Config::sApplicationID), "restart notice did not clear");

	log.str("");
	for (auto* setting : { &Config::sDetails, &Config::sState, &Config::sLargeText, &Config::sSmallText,
			 &Config::sCombatSmallText, &Config::sLabelLevel, &Config::sLabelBarterNamed, &Config::sLabelWorkbenchNamed })
	{
		for (const auto source : { "{", "{unknown}" })
		{
			setting->SetValue(source);
			rebuild();
			ExpectFeedback(*setting, Severity::kError, true);
			Require(setting->GetValue() == source, "invalid template draft overwritten");
		}
		setting->SetValue("");
		rebuild();
		Require(!Config::GetFeedback(*setting), "empty template rejected");
		setting->SetValue(setting->GetValueDefault());
		rebuild();
		Require(!Config::GetFeedback(*setting), "reset left stale feedback");
	}
	for (auto* setting : { &Config::sAssetDefault, &Config::sAssetMainMenu, &Config::sAssetLoading,
			 &Config::sAssetCharacterCreation, &Config::sAssetPlayer, &Config::sAssetCombat,
			 &Config::sAssetPowerArmor, &Config::sAssetIrradiated })
	{
		setting->SetValue("Bad-Key");
		rebuild();
		ExpectFeedback(*setting, Severity::kError, true);
		Require(setting->GetValue() == "Bad-Key", "invalid asset draft overwritten");
		setting->SetValue("");
		rebuild();
		Require(!Config::GetFeedback(*setting), "empty asset rejected");
	}
	Config::sDetails.SetValue(std::string(Presence::kFormatTemplateSourceLimit, 'x'));
	Config::sAssetDefault.SetValue("custom_art");
	rebuild();
	const auto previous = Config::Current();
	Config::sDetails.SetValue(std::string(Presence::kFormatTemplateSourceLimit + 1, 'x'));
	Config::sAssetDefault.SetValue("Bad-Key");
	Config::bShowQuest.SetValue(false);
	rebuild();
	Require(Config::Current()->details.Render({}) == previous->details.Render({}) &&
				Config::Current()->GetAssetKey(Presence::Asset::kFallout4) == "custom_art" &&
				!Config::Current()->showQuest,
		"invalid fields replaced runtime state or blocked valid edits");
	for (const auto point : { "\xC3\xA9"sv, "\xE2\x80\xA6"sv, "\xF0\x9F\x98\x80"sv })
	{
		std::string source = "{";
		for (std::size_t i = 0; i < (Presence::kFormatTemplateSourceLimit - 2) / point.size(); ++i)
			source += point;
		source += "}";
		Config::sDetails.SetValue(source);
		rebuild();
		const auto feedback = Config::GetFeedback(Config::sDetails);
		Require(feedback && feedback->message == "Invalid template at byte 0: unknown token " + source + "." &&
					feedback->message.size() <= DMUI_FIELD_FEEDBACK_MAX_MESSAGE_BYTES,
			"Unicode feedback was truncated");
	}
	const auto customBefore = Read(custom);
	const auto baseBefore = Read(base);
	Require(log.str().empty(), "quiet editing logged");
	Require(Config::SaveOverrides() == Result::kInvalidDraft &&
				Read(custom) == customBefore && Read(base) == baseBefore,
		"invalid draft changed persisted files");
	Config::Rebuild(Config::Validation::kStrict);
	Require(!Config::HasBlockingFeedback() &&
				Config::sDetails.GetValue() == Config::sDetails.GetValueDefault() &&
				Config::sAssetDefault.GetValue() == Config::sAssetDefault.GetValueDefault(),
		"strict fallback regressed");

	for (auto* setting : { &Config::iSamplingIntervalMs, &Config::iIrradiatedPercent, &Config::iMarkerMaxDistance })
	{
		setting->SetValue(1);
		rebuild();
		const auto valid = Config::Current();
		setting->SetValue(0);
		rebuild();
		ExpectFeedback(*setting, Severity::kWarning, true);
		Require(setting->GetValue() == 0 &&
					Config::Current()->samplingInterval == valid->samplingInterval &&
					Config::Current()->irradiatedPercent == valid->irradiatedPercent &&
					Config::Current()->markerMaxDistance == valid->markerMaxDistance,
			"invalid numeric draft replaced runtime state");
		Require(Config::SaveOverrides() == Result::kInvalidDraft, "numeric warning did not block saving");
		Config::Rebuild(Config::Validation::kStrict);
		Require(!Config::GetFeedback(*setting), "numeric strict fallback left stale feedback");
	}
	Config::iSamplingIntervalMs.SetValue((std::numeric_limits<std::int32_t>::max)());
	Config::iIrradiatedPercent.SetValue(Config::kMaximumIrradiatedPercent);
	Config::iMarkerMaxDistance.SetValue(Config::kMaximumMarkerMaxDistance);
	rebuild();
	Require(!Config::HasBlockingFeedback(), "numeric upper bounds rejected");
	Config::iIrradiatedPercent.SetValue(Config::kMaximumIrradiatedPercent + 1);
	Config::iMarkerMaxDistance.SetValue(Config::kMaximumMarkerMaxDistance + 1);
	rebuild();
	ExpectFeedback(Config::iIrradiatedPercent, Severity::kWarning, true);
	ExpectFeedback(Config::iMarkerMaxDistance, Severity::kWarning, true);
	Config::Rebuild(Config::Validation::kStrict);

	Config::sLabelMainMenu.SetValue("");
	rebuild();
	Require(Config::SaveOverrides() == Result::kSuccess && Read(custom).contains("keep = \"yes\"") &&
				Read(base) == baseBefore && Config::Current()->labelMainMenu.empty(),
		"valid save or unknown-key preservation regressed");
	Write(custom, "[broken");
	Require(Config::SaveOverrides() == Result::kIOError && Read(custom) == "[broken", "malformed file was overwritten");
	Write(custom, "");
	Write(base, "[Discord]\nsApplicationID = \"bad\"\n");
	Config::Load();
	ExpectFeedback(Config::sApplicationID, Severity::kError, true);
	Require(Config::GetApplicationID() == "bad", "bad startup application ID was silently replaced");
	std::cout << "ALL TESTS PASSED\n";
	return 0;
}
catch (const std::exception& a_error)
{
	std::cerr << "FAIL " << a_error.what() << '\n';
	return 1;
}
