#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>
#include <utility>

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

	struct StateBadgeLabels
	{
		std::string_view inGame;
		std::string_view inCombat;
		std::string_view inPowerArmor;
		std::string_view irradiated;
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

	template <std::equality_comparable T>
	class DebouncedValue
	{
	public:
		void Update(T a_raw)
		{
			if (a_raw == published_)
			{
				pending_ = {};
				pendingSamples_ = 0;
				return;
			}

			if (pendingSamples_ == 0 || a_raw != pending_)
			{
				pending_ = std::move(a_raw);
				pendingSamples_ = 1;
			}
			else if (pendingSamples_ < kStateBadgeSampleThreshold)
			{
				++pendingSamples_;
			}

			if (pendingSamples_ >= kStateBadgeSampleThreshold)
			{
				published_ = std::move(pending_);
				pending_ = {};
				pendingSamples_ = 0;
			}
		}

		[[nodiscard]] const T& Get() const noexcept { return published_; }

		void Reset() noexcept
		{
			published_ = {};
			pending_ = {};
			pendingSamples_ = 0;
		}

	private:
		T            published_{};
		T            pending_{};
		std::uint8_t pendingSamples_{ 0 };
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

	[[nodiscard]] inline std::string_view StateBadgeLabel(
		StateBadge              a_badge,
		const StateBadgeLabels& a_labels) noexcept
	{
		switch (a_badge)
		{
			case StateBadge::kCombat:
				return a_labels.inCombat;
			case StateBadge::kPowerArmor:
				return a_labels.inPowerArmor;
			case StateBadge::kIrradiated:
				return a_labels.irradiated;
			case StateBadge::kNone:
				return a_labels.inGame;
		}

		return a_labels.inGame;
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
