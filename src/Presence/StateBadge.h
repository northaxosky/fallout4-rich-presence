#pragma once

#include <cstdint>
#include <string_view>

namespace Presence
{
	inline constexpr std::uint8_t kStateBadgeSampleThreshold = 2;

	enum class StateBadge : std::uint8_t
	{
		kNone,
		kCombat,
		kPowerArmor,
		kIrradiated
	};

	class DebouncedFlag
	{
	public:
		void Update(bool a_raw) noexcept
		{
			if (a_raw == active_)
			{
				transitionSamples_ = 0;
				return;
			}

			if (transitionSamples_ < kStateBadgeSampleThreshold)
			{
				++transitionSamples_;
			}

			if (transitionSamples_ >= kStateBadgeSampleThreshold)
			{
				active_ = a_raw;
				transitionSamples_ = 0;
			}
		}

		void Reset() noexcept
		{
			active_ = false;
			transitionSamples_ = 0;
		}

		[[nodiscard]] bool IsActive() const noexcept { return active_; }

	private:
		bool         active_{ false };
		std::uint8_t transitionSamples_{ 0 };
	};

	[[nodiscard]] constexpr StateBadge ResolveStateBadge(
		bool a_inCombat,
		bool a_inPowerArmor,
		bool a_irradiated) noexcept
	{
		if (a_inCombat)
		{
			return StateBadge::kCombat;
		}
		if (a_inPowerArmor)
		{
			return StateBadge::kPowerArmor;
		}
		if (a_irradiated)
		{
			return StateBadge::kIrradiated;
		}
		return StateBadge::kNone;
	}

	[[nodiscard]] constexpr std::string_view StateBadgeLabel(StateBadge a_badge) noexcept
	{
		switch (a_badge)
		{
			case StateBadge::kCombat:
				return "In Combat";
			case StateBadge::kPowerArmor:
				return "In Power Armor";
			case StateBadge::kIrradiated:
				return "Irradiated";
			case StateBadge::kNone:
				return "In Game";
		}

		return "In Game";
	}

	[[nodiscard]] constexpr std::string_view ToString(StateBadge a_badge) noexcept
	{
		switch (a_badge)
		{
			case StateBadge::kNone:
				return "none";
			case StateBadge::kCombat:
				return "combat";
			case StateBadge::kPowerArmor:
				return "power_armor";
			case StateBadge::kIrradiated:
				return "irradiated";
		}

		return "none";
	}
}
