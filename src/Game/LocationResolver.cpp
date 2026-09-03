#include "pch.h"

#include "Game/LocationResolver.h"

#include <string>
#include <utility>

namespace
{
	// guards against a cyclic parentLoc chain in authored data
	inline constexpr int kMaxLocationDepth = 32;

	[[nodiscard]] std::string CopyName(const auto* a_form)
	{
		if (!a_form)
		{
			return {};
		}

		const auto name = a_form->GetFullName();
		return name && *name ? std::string{ name } : std::string{};
	}

	[[nodiscard]] std::string ReadLocation(RE::PlayerCharacter& a_player)
	{
		std::string name;
		auto        depth = 0;
		for (auto location = a_player.GetCurrentLocation(); location && name.empty() && depth < kMaxLocationDepth; location = location->parentLoc)
		{
			name = CopyName(location);
			++depth;
		}

		return name;
	}

}

namespace Game
{
	RE::TESWorldSpace* ReadWorldspace(RE::PlayerCharacter& a_player) noexcept
	{
		auto worldspace = a_player.cachedWorldspace;
		if (!worldspace)
		{
			// the cell union holds tempDataOffset when interior, so the guard is required
			const auto cell = a_player.parentCell;
			if (cell && cell->IsExterior())
			{
				worldspace = cell->worldSpace;
			}
		}

		return worldspace;
	}

	void LocationResolver::Invalidate()
	{
		latched_ = {};
		emptySamples_ = 0;
		ResetPending();
	}

	void LocationResolver::ResetPending() noexcept
	{
		pending_ = {};
		pendingSamples_ = 0;
	}

	void LocationResolver::Update(LocationDetails a_current)
	{
		const auto hasLocation = !a_current.location.empty();
		const auto hasWorldspace = !a_current.worldspace.empty();
		if (!hasLocation && !hasWorldspace)
		{
			ResetPending();
			if (emptySamples_ < kSettleSampleThreshold && ++emptySamples_ == kSettleSampleThreshold)
			{
				latched_ = {};
			}
			return;
		}

		emptySamples_ = 0;
		if ((latched_.location.empty() && latched_.worldspace.empty()) ||
			(hasLocation && hasWorldspace) ||
			a_current == latched_)
		{
			latched_ = std::move(a_current);
			ResetPending();
			return;
		}

		if (a_current == pending_)
		{
			if (pendingSamples_ < kSettleSampleThreshold)
			{
				++pendingSamples_;
			}
		}
		else
		{
			pending_ = std::move(a_current);
			pendingSamples_ = 1;
		}

		if (pendingSamples_ >= kSettleSampleThreshold)
		{
			latched_ = std::move(pending_);
			ResetPending();
		}
	}

	LocationDetails LocationResolver::Resolve(RE::PlayerCharacter* a_player, std::string_view a_nearestMarkerName)
	{
		LocationDetails current;
		if (a_player)
		{
			current.location = ReadLocation(*a_player);
			current.worldspace = CopyName(ReadWorldspace(*a_player));
			const auto cell = a_player->parentCell;
			if (current.location.empty() &&
				cell && cell->IsInterior() &&
				!a_nearestMarkerName.empty())
			{
				current.location = a_nearestMarkerName;
			}
		}

		if (!current.worldspace.empty() && current.worldspace == current.location)
		{
			current.worldspace.clear();
		}

		Update(std::move(current));
		return latched_;
	}
}
