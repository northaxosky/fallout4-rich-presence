#include "pch.h"

#include "Game/PlayerState.h"

#include <RE/P/PowerArmor.h>

#include <algorithm>
#include <cmath>
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
