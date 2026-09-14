#include "Host/Navigation.h"

#include <DearModdingUI/IconGlyphs.h>

#include <iostream>
#include <string_view>

namespace
{
	template <class... Arguments>
	DMUI_Result DMUI_CALL UnusedOperation(Arguments...) noexcept
	{
		return DMUI_RESULT_OK;
	}

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
	passed &= Check(Host::kClientOptions.capabilities == DMUI_CLIENT_CAPABILITY_NONE &&
						Host::kClientOptions.requiredServices == DMUI_HOST_SERVICE_EXTERNAL_OPEN &&
						Host::kClientOptions.minimumUIRevision == DMUI_UI_REVISION_1 &&
						Host::kClientOptions.minimumUIAPISize == DMUI_UI_API_REQUIRED_SIZE,
		"required host services and UI prefix");

	DMUI_HostAPI api{};
	api.structSize = DMUI_HOST_API_END_FIELD_SIZE;
	api.beginField = &UnusedOperation;
	api.setFieldFeedback = &UnusedOperation;
	api.endField = &UnusedOperation;
	passed &= Check(Host::HasFieldFeedbackAPI(&api), "exact feedback prefix is accepted");
	passed &= Check(!Host::HasFieldFeedbackAPI(nullptr), "absent host is rejected");
	for (const auto size : { DMUI_HOST_API_BEGIN_FIELD_SIZE - 1,
			 DMUI_HOST_API_SET_FIELD_FEEDBACK_SIZE - 1, DMUI_HOST_API_END_FIELD_SIZE - 1 })
	{
		api.structSize = size;
		passed &= Check(!Host::HasFieldFeedbackAPI(&api), "truncated feedback table is rejected");
	}
	api.structSize = DMUI_HOST_API_END_FIELD_SIZE;
	api.beginField = nullptr;
	passed &= Check(!Host::HasFieldFeedbackAPI(&api), "missing beginField is rejected");
	api.beginField = &UnusedOperation;
	api.setFieldFeedback = nullptr;
	passed &= Check(!Host::HasFieldFeedbackAPI(&api), "missing setFieldFeedback is rejected");
	api.setFieldFeedback = &UnusedOperation;
	api.endField = nullptr;
	passed &= Check(!Host::HasFieldFeedbackAPI(&api), "missing endField is rejected");

	if (passed)
		std::cout << "ALL TESTS PASSED\n";
	return passed ? 0 : 1;
}
