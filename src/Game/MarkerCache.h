#pragma once

#include <RE/M/MARKER_TYPE.h>
#include <RE/N/NiPoint3.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace RE
{
	class PlayerCharacter;
	class TESObjectREFR;
	class TESWorldSpace;
}

namespace Game
{
	struct MarkerDetails
	{
		RE::MARKER_TYPE type;
		std::string     name;

		bool operator==(const MarkerDetails&) const = default;
	};

	class MarkerCache
	{
	public:
		void Build();
		void Invalidate();

		// mirrors LocationResolver: query, settle, then return the latched result
		[[nodiscard]] std::optional<MarkerDetails> Resolve(RE::PlayerCharacter* a_player, float a_maxDistance);

	private:
		// a Voronoi boundary between two markers would otherwise flip the large image every sample
		static constexpr std::uint8_t kSettleSampleThreshold = 3;

		struct Entry
		{
			RE::TESObjectREFR* reference;
			RE::NiPoint3       position;
			RE::TESWorldSpace* worldspace;
			RE::MARKER_TYPE    type;
		};

		[[nodiscard]] std::optional<MarkerDetails> FindNearest(RE::PlayerCharacter* a_player, float a_maxDistance) const;

		// entries_ is static game data built once; the latch below is per-session and resets on load
		mutable std::mutex           mutex_;
		std::vector<Entry>           entries_;
		std::optional<MarkerDetails> latched_;
		std::optional<MarkerDetails> pending_;
		std::uint8_t                 pendingSamples_{ 0 };
	};
}
