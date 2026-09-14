#pragma once

#include <DearModdingUI/Client.h>

namespace Host
{
	inline constexpr auto kClientIcon = "discord-logo";

	[[nodiscard]] constexpr bool HasFieldFeedbackAPI(const DMUI_HostAPI* a_api) noexcept
	{
		return a_api &&
		       a_api->structSize >= DMUI_HOST_API_BEGIN_FIELD_SIZE &&
		       a_api->beginField &&
		       a_api->structSize >= DMUI_HOST_API_SET_FIELD_FEEDBACK_SIZE &&
		       a_api->setFieldFeedback &&
		       a_api->structSize >= DMUI_HOST_API_END_FIELD_SIZE &&
		       a_api->endField;
	}

	inline constexpr dmui::ClientOptions kClientOptions{
		.requiredServices = DMUI_HOST_SERVICE_EXTERNAL_OPEN,
		.minimumUIRevision = DMUI_UI_REVISION_1,
		.minimumUIAPISize = DMUI_UI_API_REQUIRED_SIZE
	};

	inline constexpr dmui::CategoryDescriptor kGeneralCategory{
		.id = "general",
		.displayName = "General",
		.sortKey = 0,
		.iconName = "gear"
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
