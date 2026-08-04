#include "pch.h"

#include "Game/PlayerState.h"

#include <string>

namespace Game
{
	PlayerState ReadPlayerState(RE::PlayerCharacter* a_player, bool a_sessionActive)
	{
		PlayerState state;

		if (const auto ui = RE::UI::GetSingleton())
		{
			state.inMainMenu = ui->GetMenuOpen<RE::MainMenu>();
			state.inLoadingMenu = ui->GetMenuOpen<RE::LoadingMenu>();
		}

		// the singleton exists from process start, but its actor data is only bound once a session does
		if (!a_player || !a_sessionActive)
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

			// Actor::GetLevel is a native call that dereferences the base NPC unguarded
			state.level = a_player->GetLevel();
		}

		state.inCombat = a_player->IsInCombat();
		state.charGenFlag = a_player->byCharGenFlag;
		state.inLooksMenu = a_player->inLooksMenu;

		return state;
	}
}
