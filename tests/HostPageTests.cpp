#include "Host/Navigation.h"

#include <DearModdingUI/IconGlyphs.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] bool Check(bool a_condition, std::string_view a_message)
	{
		if (!a_condition)
			std::cerr << "FAIL " << a_message << '\n';
		return a_condition;
	}

	[[nodiscard]] std::map<std::string, std::string, std::less<>> LoadTranslations()
	{
		std::map<std::string, std::string, std::less<>> entries;
		std::ifstream file{ std::string{ "data/Interface/Translations/" } + Host::kClientID + "_en.txt" };
		for (std::string line; std::getline(file, line);)
		{
			if (const auto tab = line.find('\t'); tab != std::string::npos)
				entries.emplace(line.substr(0, tab), line.substr(tab + 1));
		}
		return entries;
	}

	[[nodiscard]] bool HasIcon(const char* a_name)
	{
		return a_name && DearModdingUI::FindPhosphorSlugGlyphOrZero(a_name) != 0;
	}
}

int main()
{
	bool passed = true;
	passed &= Check(std::string_view{ Host::kGeneralCategory.id } == "general" &&
						Host::kGeneralCategory.sortKey == 0,
		"General category identity");
	passed &= Check(DearModdingUI::FindPhosphorSlugGlyphOrZero(Host::kGeneralCategory.iconName) ==
						DearModdingUI::PhosphorGlyph::kGear,
		"General requests the gear icon");
	passed &= Check(std::string_view{ Host::kClientIcon } == "discord-logo" && HasIcon(Host::kClientIcon),
		"Discord client logo");
	passed &= Check(HasIcon("github-logo"), "GitHub link logo");
	passed &= Check(std::string_view{ Host::kHomePage.id } == "home" && Host::kHomePage.sortKey == 0,
		"Home page identity and order");
	passed &= Check(std::string_view{ Host::kSettingsPage.id } == "settings" && Host::kSettingsPage.sortKey == 10,
		"Settings page identity and order");
	for (const auto& page : { Host::kHomePage, Host::kSettingsPage })
	{
		passed &= Check(std::string_view{ page.categoryId } == Host::kGeneralCategory.id &&
							page.kind == DMUI_PAGE_KIND_SETTINGS,
			"pages are grouped sidebar pages, not overlays");
		// translated names get no automatic icon
		passed &= Check(HasIcon(page.iconName), "pages name an explicit icon");
	}

	// registration text has no code fallback, so a missing key would show raw
	const auto translations = LoadTranslations();
	for (const auto* key : {
			 Host::kClientDisplayName,
			 Host::kGeneralCategory.displayName,
			 Host::kHomePage.displayName,
			 Host::kHomePage.summary,
			 Host::kSettingsPage.displayName,
			 Host::kSettingsPage.summary })
	{
		const auto entry = translations.find(key);
		passed &= Check(entry != translations.end() && !entry->second.empty(),
			std::string{ "English translation for " } + key);
	}

	if (passed)
		std::cout << "ALL TESTS PASSED\n";
	return passed ? 0 : 1;
}
