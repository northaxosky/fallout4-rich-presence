#pragma once

#include <DearModdingUI/Client.h>

namespace Host
{
	inline constexpr auto kClientIcon = "discord-logo";

	inline constexpr dmui::ClientOptions kClientOptions{
		.requiredServices = DMUI_HOST_SERVICE_EXTERNAL_OPEN,
		.minimumForwardingVersion = DMUI_FORWARDING_VERSION_CURRENT
	};

	inline constexpr dmui::CategoryDescriptor kGeneralCategory{
		.id = "general",
		.displayName = "General",
		.sortKey = 0
	};

	inline constexpr dmui::PageDescriptor kHomePage{
		.id = "home",
		.displayName = "Home",
		.categoryId = kGeneralCategory.id,
		.summary = "Connection state and the latest generated presence.",
		.sortKey = 0
	};

	inline constexpr dmui::PageDescriptor kSettingsPage{
		.id = "settings",
		.displayName = "Settings",
		.categoryId = kGeneralCategory.id,
		.summary = "Configure Discord Rich Presence and inspect its connection.",
		.sortKey = 10
	};
}
