#include "pch.h"

#include "Game/PlayerState.h"

#include <string>

namespace Game
{
	PlayerState ReadPlayerState(RE::PlayerCharacter* a_player)
	{
		PlayerState state;

		if (const auto ui = RE::UI::GetSingleton())
		{
			state.inMainMenu = ui->GetMenuOpen<RE::MainMenu>();
			state.inLoadingMenu = ui->GetMenuOpen<RE::LoadingMenu>();
		}

		if (!a_player)
		{
			return state;
		}

		// GetDisplayFullName allocates from the ScrapHeap; the base NPC name is the safe path
		if (const auto npc = a_player->GetNPC())
		{
			if (const auto name = npc->GetFullName(); name && *name)
			{
				state.name = name;
			}
		}

		state.level = a_player->GetLevel();
		state.inCombat = a_player->IsInCombat();
		state.charGenFlag = a_player->byCharGenFlag;
		state.inLooksMenu = a_player->inLooksMenu;

		return state;
	}
}
