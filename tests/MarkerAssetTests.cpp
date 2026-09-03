#include "Presence/AssetKeys.h"
#include "Presence/MarkerAssets.h"

#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{
	[[nodiscard]] bool IsIntendedSharedKey(
		const Presence::MarkerAsset& a_left,
		const Presence::MarkerAsset& a_right) noexcept
	{
		return a_left.key == "marker_railroad" &&
		       ((a_left.type == Presence::MarkerType::kRailroad &&
					a_right.type == Presence::MarkerType::kRailroadFaction) ||
				   (a_left.type == Presence::MarkerType::kRailroadFaction &&
					   a_right.type == Presence::MarkerType::kRailroad));
	}
}

int main()
{
	bool passed = true;

	for (const auto& mapping : Presence::kMarkerAssets)
	{
		const auto markerType = std::to_underlying(mapping.type);
		const auto resolved = Presence::MarkerAssetKey(markerType);
		if (resolved != mapping.key)
		{
			std::cerr << "FAIL marker " << markerType << ": expected \"" << mapping.key
					  << "\", got \"" << resolved << "\"\n";
			passed = false;
		}
		if (!Presence::IsValidAssetKey(mapping.key))
		{
			std::cerr << "FAIL marker " << markerType << ": invalid asset key \"" << mapping.key << "\"\n";
			passed = false;
		}
	}

	for (auto left = Presence::kMarkerAssets.begin(); left != Presence::kMarkerAssets.end(); ++left)
	{
		for (auto right = left + 1; right != Presence::kMarkerAssets.end(); ++right)
		{
			if (left->type != right->type &&
				left->key == right->key &&
				!IsIntendedSharedKey(*left, *right))
			{
				std::cerr << "FAIL duplicate key \"" << left->key << "\" for marker types "
						  << std::to_underlying(left->type) << " and "
						  << std::to_underlying(right->type) << '\n';
				passed = false;
			}
		}
	}

	for (const auto unmapped : { 0x03u, 0x53u, 0x5Bu, UINT32_MAX })
	{
		if (!Presence::MarkerAssetKey(unmapped).empty())
		{
			std::cerr << "FAIL unmapped marker " << unmapped << " returned an asset key\n";
			passed = false;
		}
	}

	if (passed)
	{
		std::cout << "ALL TESTS PASSED\n";
	}
	return passed ? 0 : 1;
}
