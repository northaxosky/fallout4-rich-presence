#pragma once

#include <cstdint>
#include <string>

namespace RE
{
	class PlayerCharacter;
}

namespace Game
{
	struct PlayerState
	{
		std::string  name;
		std::int16_t level{ 0 };
		std::int8_t  charGenFlag{ 0 };
		bool         inCombat{ false };
		bool         inMainMenu{ false };
		bool         inLoadingMenu{ false };
		bool         inLooksMenu{ false };
	};

	[[nodiscard]] PlayerState ReadPlayerState(RE::PlayerCharacter* a_player, bool a_sessionActive);
}
