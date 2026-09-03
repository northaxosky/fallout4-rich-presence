#pragma once

#include "Game/LocationResolver.h"
#include "Game/PlayerState.h"
#include "Game/QuestResolver.h"

#include <cstdint>
#include <optional>

namespace Game
{
	// every string is owned so it can outlive tick
	struct Snapshot
	{
		QuestDetails                 quest;
		LocationDetails              location;
		PlayerState                  player;
		std::optional<std::uint32_t> markerType;
	};
}
