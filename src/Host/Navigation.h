#pragma once

#include <DearModdingUI/Client.h>

namespace Host
{
	inline constexpr auto kClientID = "dearmodding.richpresence";
	inline constexpr auto kClientDisplayName = "$RichPresence_ClientName";
	inline constexpr auto kClientIcon = "discord-logo";

	// translated names skip automatic icon guessing, so every entry names its icon
	inline constexpr dmui::CategoryDescriptor kGeneralCategory{
		.id = "general",
		.displayName = "$RichPresence_CategoryGeneral",
		.sortKey = 0,
		.iconName = "gear"
	};

	inline constexpr dmui::PageDescriptor kHomePage{
		.id = "home",
		.displayName = "$RichPresence_PageHome",
		.categoryId = kGeneralCategory.id,
		.summary = "$RichPresence_PageHomeSummary",
		.sortKey = 0,
		.iconName = "house"
	};

	inline constexpr dmui::PageDescriptor kSettingsPage{
		.id = "settings",
		.displayName = "$RichPresence_PageSettings",
		.categoryId = kGeneralCategory.id,
		.summary = "$RichPresence_PageSettingsSummary",
		.sortKey = 10,
		.iconName = "sliders-horizontal"
	};
}
