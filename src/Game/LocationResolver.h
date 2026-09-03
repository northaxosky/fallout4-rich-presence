#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace RE
{
	class PlayerCharacter;
	class TESWorldSpace;
}

namespace Game
{
	[[nodiscard]] RE::TESWorldSpace* ReadWorldspace(RE::PlayerCharacter& a_player) noexcept;

	struct LocationDetails
	{
		std::string location;
		std::string worldspace;

		bool operator==(const LocationDetails&) const = default;
	};

	class LocationResolver
	{
	public:
		[[nodiscard]] LocationDetails Resolve(RE::PlayerCharacter* a_player, std::string_view a_nearestMarkerName = {});

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
