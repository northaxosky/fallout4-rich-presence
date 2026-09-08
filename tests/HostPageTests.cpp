#include "Host/Navigation.h"

#include <DearModdingUI/IconGlyphs.h>

#include <iostream>
#include <string_view>

namespace
{
	DMUI_HostServices g_services{ DMUI_HOST_SERVICE_EXTERNAL_OPEN };
	std::uint32_t     g_forwardingVersion{ DMUI_FORWARDING_VERSION_CURRENT };
	std::uint32_t     g_registrations{ 0 };

	DMUI_Result DMUI_CALL RegisterClient(const DMUI_ClientDescriptor*, DMUI_ClientHandle*) noexcept
	{
		++g_registrations;
		return DMUI_RESULT_OK;
	}

	DMUI_Result DMUI_CALL QueryServices(DMUI_HostServicesInfo* a_services) noexcept
	{
		a_services->forwardingVersion = g_forwardingVersion;
		a_services->supportedServices = g_services;
		return DMUI_RESULT_OK;
	}

	DMUI_Result DMUI_CALL OpenExternal(DMUI_ClientHandle, const DMUI_ExternalOpenDescriptor*, std::uint32_t*) noexcept
	{
		return DMUI_RESULT_OK;
	}

	[[nodiscard]] bool Check(bool a_condition, std::string_view a_message)
	{
		if (!a_condition)
		{
			std::cerr << "FAIL " << a_message << '\n';
		}
		return a_condition;
	}
}

int main()
{
	bool passed = true;
	passed &= Check(std::string_view{ Host::kGeneralCategory.id } == "general", "stable general category ID");
	passed &= Check(std::string_view{ Host::kGeneralCategory.displayName } == "General", "visible General heading");
	passed &= Check(Host::kGeneralCategory.sortKey == 0, "General category order");
	const auto discordGlyph = DearModdingUI::FindPhosphorSlugGlyphOrZero("discord-logo");
	passed &= Check(discordGlyph != 0 &&
						DearModdingUI::ResolveClientIconGlyph(Host::kClientIcon, {}, "Rich Presence") == discordGlyph,
		"client uses the Discord logo");
	passed &= Check(DearModdingUI::ResolveCategoryIconGlyph(
						Host::kGeneralCategory.displayName, "Rich Presence", "dearmodding.richpresence", Host::kClientIcon) ==
						DearModdingUI::PhosphorGlyph::kGear,
		"General resolves to the host gear icon");
	passed &= Check(DearModdingUI::FindPhosphorSlugGlyphOrZero("github-logo") != 0,
		"GitHub logo exists in the host icon catalog");
	passed &= Check(std::string_view{ Host::kHomePage.id } == "home" &&
						std::string_view{ Host::kHomePage.displayName } == "Home",
		"Home page identity");
	passed &= Check(std::string_view{ Host::kSettingsPage.id } == "settings" &&
						std::string_view{ Host::kSettingsPage.displayName } == "Settings",
		"Settings page identity");
	for (const auto& page : { Host::kHomePage, Host::kSettingsPage })
	{
		passed &= Check(std::string_view{ page.categoryId } == Host::kGeneralCategory.id,
			"both pages reference the declared category");
		passed &= Check(page.kind == DMUI_PAGE_KIND_SETTINGS, "pages belong in the sidebar, not overlays");
	}
	passed &= Check(Host::kHomePage.sortKey < Host::kSettingsPage.sortKey, "Home precedes Settings");
	passed &= Check(Host::kClientOptions.capabilities == DMUI_CLIENT_CAPABILITY_NONE,
		"the client does not replace the renderer");
	passed &= Check(Host::kClientOptions.requiredServices == DMUI_HOST_SERVICE_EXTERNAL_OPEN,
		"browser links require external opening");
	passed &= Check(Host::kClientOptions.minimumForwardingVersion == DMUI_FORWARDING_VERSION_CURRENT,
		"the client requires the pinned forwarding surface");

	DMUI_HostAPI api{};
	api.structSize = sizeof(api);
	api.registerClient = &RegisterClient;
	api.queryServices = &QueryServices;
	api.openExternal = &OpenExternal;
	passed &= Check(dmui::PreflightHostAPI(&api, Host::kClientOptions) == DMUI_RESULT_OK,
		"compatible services pass preflight");
	passed &= Check(dmui::PreflightHostAPI(nullptr, Host::kClientOptions) == DMUI_RESULT_UNSUPPORTED_ABI,
		"absent API fails preflight");

	g_services = DMUI_HOST_SERVICE_NONE;
	passed &= Check(dmui::PreflightHostAPI(&api, Host::kClientOptions) == DMUI_RESULT_SERVICE_UNAVAILABLE,
		"host without external opening is rejected");
	g_services = DMUI_HOST_SERVICE_EXTERNAL_OPEN;
	g_forwardingVersion = DMUI_FORWARDING_VERSION_1_0;
	passed &= Check(dmui::PreflightHostAPI(&api, Host::kClientOptions) == DMUI_RESULT_FORWARDING_VERSION_MISMATCH,
		"old forwarding surface is rejected");
	g_forwardingVersion = DMUI_FORWARDING_VERSION_CURRENT;

	api.openExternal = nullptr;
	passed &= Check(dmui::PreflightHostAPI(&api, Host::kClientOptions) == DMUI_RESULT_SERVICE_UNAVAILABLE,
		"advertised service without its entry point is rejected");
	api.openExternal = &OpenExternal;
	api.structSize = DMUI_HOST_API_OPEN_EXTERNAL_SIZE - 1;
	passed &= Check(dmui::PreflightHostAPI(&api, Host::kClientOptions) == DMUI_RESULT_SERVICE_UNAVAILABLE,
		"truncated external-open table is rejected");
	passed &= Check(g_registrations == 0, "preflight never registers a partial client");

	if (passed)
	{
		std::cout << "ALL TESTS PASSED\n";
	}
	return passed ? 0 : 1;
}
