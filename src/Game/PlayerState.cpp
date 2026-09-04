#include "pch.h"

#include "Game/PlayerState.h"
#include "Presence/MenuActivity.h"

#include <RE/P/PowerArmor.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	[[nodiscard]] bool HasVisibleName(const char* a_name) noexcept
	{
		if (!a_name)
		{
			return false;
		}

		return std::string_view{ a_name }.find_first_not_of(" \t\n\r\f\v") != std::string_view::npos;
	}

	[[nodiscard]] std::string CopyVisibleName(const RE::TESFullName* a_fullName)
	{
		const auto name = a_fullName ? a_fullName->GetFullName() : nullptr;
		return HasVisibleName(name) ? std::string{ name } : std::string{};
	}

	[[nodiscard]] std::string ReadActorBaseName(RE::TESObjectREFR* a_reference)
	{
		const auto baseObject = a_reference ? a_reference->GetObjectReference() : nullptr;
		const auto actorBase = baseObject ? baseObject->As<RE::TESActorBase>() : nullptr;
		return CopyVisibleName(actorBase);
	}

	[[nodiscard]] std::string ReadFurnitureBaseName(RE::TESObjectREFR* a_reference)
	{
		const auto baseObject = a_reference ? a_reference->GetObjectReference() : nullptr;
		const auto furniture = baseObject ? baseObject->As<RE::TESFurniture>() : nullptr;
		return CopyVisibleName(furniture);
	}

	[[nodiscard]] RE::TESObjectREFR* ResolveHandle(const RE::ObjectRefHandle& a_handle)
	{
		const auto reference = a_handle.get();
		return reference ? reference.get() : nullptr;
	}

	[[nodiscard]] RE::TESObjectREFR* ReadOccupiedWorkbench(RE::PlayerCharacter& a_player)
	{
		if (!a_player.currentProcess)
		{
			return nullptr;
		}

		const auto reference = ResolveHandle(a_player.currentProcess->GetOccupiedFurniture());
		const auto baseObject = reference ? reference->GetObjectReference() : nullptr;
		const auto furniture = baseObject ? baseObject->As<RE::TESFurniture>() : nullptr;
		return furniture && furniture->wbData.type.get() != RE::WorkbenchData::Type::kNone ?
		           reference :
		           nullptr;
	}

	void ReadMenuActivity(
		Game::PlayerState&   a_state,
		RE::PlayerCharacter& a_player,
		const RE::UI&        a_ui)
	{
		using Presence::MenuActivity;

		if (a_ui.GetMenuOpen<RE::BarterMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kBarter);
			const auto menu = a_ui.GetMenu<RE::BarterMenu>();
			if (menu)
			{
				a_state.menuActivityName = ReadActorBaseName(ResolveHandle(menu->vendorActor));
			}
			return;
		}

		// WorkbenchMenuBase has no menu name, so occupied workbench furniture is the stable signal.
		if (const auto workbench = ReadOccupiedWorkbench(a_player))
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kWorkbench);
			a_state.menuActivityName = ReadFurnitureBaseName(workbench);
			return;
		}

		if (a_ui.GetMenuOpen<RE::WorkshopMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kWorkshop);
		}
		else if (a_ui.GetMenuOpen<RE::TerminalMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kTerminal);
		}
		else if (a_ui.GetMenuOpen<RE::LockpickingMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kLockpicking);
		}
		else if (a_ui.GetMenuOpen<RE::SitWaitMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kSitWait);
		}
		else if (a_ui.GetMenuOpen<RE::DialogueMenu>())
		{
			a_state.menuActivity = std::to_underlying(MenuActivity::kDialogue);
		}
	}
}

namespace Game
{
	PlayerState ReadPlayerState(RE::PlayerCharacter* a_player, bool a_sessionActive)
	{
		PlayerState state;

		const auto ui = RE::UI::GetSingleton();
		if (ui)
		{
			state.inMainMenu = ui->GetMenuOpen<RE::MainMenu>();
			state.inLoadingMenu = ui->GetMenuOpen<RE::LoadingMenu>();
		}

		// the singleton exists from process start, but its actor data is only bound once a session does
		if (!a_player || !a_sessionActive)
		{
			return state;
		}

		if (ui && !state.inMainMenu && !state.inLoadingMenu)
		{
			ReadMenuActivity(state, *a_player, *ui);
		}

		// GetDisplayFullName allocates from the ScrapHeap; the base NPC name is the safe path
		if (const auto npc = a_player->GetNPC())
		{
			if (const auto name = npc->GetFullName(); name && *name)
			{
				state.name = name;
			}

			// Actor::GetLevel is a native call that dereferences the base NPC unguarded
			state.level = a_player->GetLevel();
		}

		state.inCombat = a_player->IsInCombat();
		if (state.inCombat && a_player->currentCombatTarget)
		{
			const auto target = a_player->currentCombatTarget.get();
			if (target)
			{
				state.combatTargetID = target->GetFormID();

				state.combatTargetName = ReadActorBaseName(target.get());
			}
		}
		state.inPowerArmor = RE::PowerArmor::PlayerInPowerArmor();
		if (const auto actorValues = RE::ActorValue::GetSingleton();
			actorValues && actorValues->rads && actorValues->health)
		{
			const auto rads = a_player->GetActorValue(*actorValues->rads);
			const auto health = a_player->GetActorValue(*actorValues->health);
			const auto totalHealth = health + rads;
			if (totalHealth > 0.0F)
			{
				const auto fraction = rads / totalHealth;
				if (std::isfinite(fraction))
				{
					state.radsFraction = std::clamp(fraction, 0.0F, 1.0F);
				}
			}
		}
		state.charGenFlag = a_player->byCharGenFlag;
		state.inLooksMenu = a_player->inLooksMenu;

		return state;
	}
}
