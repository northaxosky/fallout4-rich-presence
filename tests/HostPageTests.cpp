#include "Host/Navigation.h"

#include <DearModdingUI/IconGlyphs.h>

#include <iostream>
#include <string_view>

namespace
{
	[[nodiscard]] bool Check(bool a_condition, std::string_view a_message)
	{
		if (!a_condition)
			std::cerr << "FAIL " << a_message << '\n';
		return a_condition;
	}
}

int main()
{
	bool passed = true;
	passed &= Check(std::string_view{ Host::kGeneralCategory.id } == "general" &&
						std::string_view{ Host::kGeneralCategory.displayName } == "General" &&
						Host::kGeneralCategory.sortKey == 0,
		"General category identity");
	passed &= Check(std::string_view{ Host::kGeneralCategory.iconName } == "gear" &&
						DearModdingUI::FindPhosphorSlugGlyphOrZero(Host::kGeneralCategory.iconName) ==
							DearModdingUI::PhosphorGlyph::kGear,
		"General requests the gear icon");
	passed &= Check(std::string_view{ Host::kClientIcon } == "discord-logo" &&
						DearModdingUI::FindPhosphorSlugGlyphOrZero(Host::kClientIcon) != 0,
		"Discord client logo");
	passed &= Check(DearModdingUI::FindPhosphorSlugGlyphOrZero("github-logo") != 0, "GitHub link logo");
	passed &= Check(std::string_view{ Host::kHomePage.id } == "home" &&
						std::string_view{ Host::kHomePage.displayName } == "Home" &&
						Host::kHomePage.sortKey == 0,
		"Home page identity and order");
	passed &= Check(std::string_view{ Host::kSettingsPage.id } == "settings" &&
						std::string_view{ Host::kSettingsPage.displayName } == "Settings" &&
						Host::kSettingsPage.sortKey == 10,
		"Settings page identity and order");
	for (const auto& page : { Host::kHomePage, Host::kSettingsPage })
	{
		passed &= Check(std::string_view{ page.categoryId } == Host::kGeneralCategory.id &&
							page.kind == DMUI_PAGE_KIND_SETTINGS,
			"pages are grouped sidebar pages, not overlays");
	}
	if (passed)
		std::cout << "ALL TESTS PASSED\n";
	return passed ? 0 : 1;
}
