#include "Presence/MarkerAssets.h"

#include "Presence/AssetKeys.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace
{
	consteval bool AreMarkerAssetKeysValid()
	{
		return std::ranges::all_of(Presence::kMarkerAssets, [](const auto& a_mapping) {
			return !a_mapping.key.empty() && Presence::IsValidAssetKey(a_mapping.key);
		});
	}

	static_assert(AreMarkerAssetKeysValid());
}

namespace Presence
{
	std::string_view MarkerAssetKey(std::uint32_t a_markerType) noexcept
	{
		const auto mapping = std::ranges::find_if(kMarkerAssets, [a_markerType](const auto& a_mapping) {
			return std::to_underlying(a_mapping.type) == a_markerType;
		});
		return mapping != kMarkerAssets.end() ? mapping->key : std::string_view{};
	}
}
