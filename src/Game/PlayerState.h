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
		std::string   name;
		std::string   combatTargetName;
		std::uint32_t combatTargetID{ 0 };
		std::int16_t  level{ 0 };
		std::int8_t   charGenFlag{ 0 };
		std::uint8_t  menuActivity{ 0 };
		std::string   menuActivityName;
		float         radsFraction{ 0.0F };
		bool          inCombat{ false };
		bool          inPowerArmor{ false };
		bool          inMainMenu{ false };
		bool          inLoadingMenu{ false };
		bool          inLooksMenu{ false };
	};

	[[nodiscard]] PlayerState ReadPlayerState(RE::PlayerCharacter* a_player, bool a_sessionActive);
}
