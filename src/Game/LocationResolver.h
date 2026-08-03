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

		bool operator==(const LocationDetails&) const = default;
	};

	class LocationResolver
	{
	public:
		[[nodiscard]] LocationDetails Resolve(RE::PlayerCharacter* a_player);

		void Invalidate();

	private:
		// hold the composed pair so independently stale fields can never be combined
		static constexpr std::uint8_t kSettleSampleThreshold = 3;

		void ResetPending() noexcept;
		void Update(LocationDetails a_current);

		LocationDetails latched_;
		LocationDetails pending_;
		std::uint8_t    emptySamples_{ 0 };
		std::uint8_t    pendingSamples_{ 0 };
	};
}
