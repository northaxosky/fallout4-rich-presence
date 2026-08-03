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

	[[nodiscard]] constexpr std::string_view ToAssetKey(Asset a_asset) noexcept
	{
		switch (a_asset)
		{
			case Asset::kFallout4:
				return "fallout4";
			case Asset::kMainMenu:
				return "main_menu";
			case Asset::kLoading:
				return "loading";
			case Asset::kCharacterCreation:
				return "character_creation";
			case Asset::kPlayer:
				return "player";
			case Asset::kCombat:
				return "combat";
		}

		return {};
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

	static_assert(IsValidAssetKey(ToAssetKey(Asset::kFallout4)));
	static_assert(IsValidAssetKey(ToAssetKey(Asset::kMainMenu)));
	static_assert(IsValidAssetKey(ToAssetKey(Asset::kLoading)));
	static_assert(IsValidAssetKey(ToAssetKey(Asset::kCharacterCreation)));
	static_assert(IsValidAssetKey(ToAssetKey(Asset::kPlayer)));
	static_assert(IsValidAssetKey(ToAssetKey(Asset::kCombat)));
}
