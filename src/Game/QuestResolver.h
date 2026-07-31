#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace RE
{
	class PlayerCharacter;
}

namespace Game
{
	struct QuestDetails
	{
		std::string   title;
		std::string   objective;
		std::uint32_t formID{ 0 };
		std::uint32_t instanceID{ 0 };
		std::uint16_t objectiveIndex{ 0 };
		std::int8_t   priority{ 0 };
		bool          hasQuest{ false };
		bool          hasObjective{ false };
	};

	class QuestResolver
	{
	public:
		[[nodiscard]] QuestDetails Resolve(RE::PlayerCharacter* a_player);

		void Invalidate();

	private:
		struct Identity
		{
			std::uint32_t questFormID;
			std::uint32_t instanceID;
			std::uint16_t objectiveIndex;

			bool operator==(const Identity&) const = default;
		};

		std::optional<Identity> cachedIdentity_;
		QuestDetails            cachedDetails_;
	};
}
