#pragma once

#include <cstdint>
#include <string>

namespace RE
{
	class PlayerCharacter;
}

namespace Game
{
	struct LocationDetails
	{
		std::string location;
		std::string worldspace;
	};

	class LocationResolver
	{
	public:
		[[nodiscard]] LocationDetails Resolve(RE::PlayerCharacter* a_player);

		void Invalidate();

	private:
		// a read can return empty mid-transition, so hold the last good value briefly
		static constexpr std::uint8_t kEmptySampleThreshold = 3;

		struct Latched
		{
			std::string  value;
			std::uint8_t emptySamples{ 0 };
		};

		void Update(Latched& a_latched, std::string a_current) const;

		Latched location_;
		Latched worldspace_;
	};
}
