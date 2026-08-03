#pragma once

#include <cstdint>
#include <string_view>

namespace Presence
{
	enum class Asset : std::uint8_t
	{
		kFallout4,
		kMainMenu,
		kLoading,
		kCharacterCreation,
		kPlayer,
		kCombat
	};

	inline constexpr std::string_view kDefaultAssetKey = "fallout4";

	[[nodiscard]] constexpr std::string_view DefaultAssetKey(Asset) noexcept
	{
		return kDefaultAssetKey;
	}

	[[nodiscard]] constexpr bool IsValidAssetKey(std::string_view a_key) noexcept
	{
		if (a_key.empty() || a_key.size() > 32)
		{
			return false;
		}

		for (const auto character : a_key)
		{
			if ((character < 'a' || character > 'z') &&
				(character < '0' || character > '9') &&
				character != '_')
			{
				return false;
			}
		}

		return true;
	}

	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kFallout4)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kMainMenu)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kLoading)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kCharacterCreation)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kPlayer)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kCombat)));
}
