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
		kCombat,
		kPowerArmor,
		kIrradiated
	};

	inline constexpr std::string_view kDefaultAssetKey = "fallout4";

	// slots without their own uploaded art fall back to the default image
	[[nodiscard]] constexpr std::string_view DefaultAssetKey(Asset a_asset) noexcept
	{
		switch (a_asset)
		{
			case Asset::kMainMenu:
				return "mainmenu";
			case Asset::kPlayer:
			case Asset::kCombat:
				return "vaultboy";
			case Asset::kPowerArmor:
				return "state_powerarmor";
			case Asset::kIrradiated:
				return "state_irradiated";
			default:
				return kDefaultAssetKey;
		}
	}

	// empty means "no image for this slot"; Discord omits an absent asset rather than failing
	[[nodiscard]] constexpr bool IsValidAssetKey(std::string_view a_key) noexcept
	{
		if (a_key.size() > 32)
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

	static_assert(IsValidAssetKey(""));

	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kFallout4)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kMainMenu)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kLoading)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kCharacterCreation)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kPlayer)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kCombat)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kPowerArmor)));
	static_assert(IsValidAssetKey(DefaultAssetKey(Asset::kIrradiated)));
}
