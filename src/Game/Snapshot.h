#pragma once

#include "Game/LocationResolver.h"
#include "Game/PlayerState.h"
#include "Game/QuestResolver.h"

namespace Game
{
	// every string is owned so it can outlive tick
	struct Snapshot
	{
		QuestDetails    quest;
		LocationDetails location;
		PlayerState     player;
	};
}
