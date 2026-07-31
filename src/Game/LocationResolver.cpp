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

	[[nodiscard]] std::string ReadWorldspace(RE::PlayerCharacter& a_player)
	{
		const RE::TESWorldSpace* worldspace = a_player.cachedWorldspace;
		if (!worldspace)
		{
			// the cell union holds tempDataOffset when interior, so the guard is required
			const auto cell = a_player.parentCell;
			if (cell && cell->IsExterior())
			{
				worldspace = cell->worldSpace;
			}
		}

		return CopyName(worldspace);
	}
}

namespace Game
{
	void LocationResolver::Invalidate()
	{
		location_ = {};
		worldspace_ = {};
	}

	void LocationResolver::Update(Latched& a_latched, std::string a_current) const
	{
		if (!a_current.empty())
		{
			a_latched.value = std::move(a_current);
			a_latched.emptySamples = 0;
		}
		else if (a_latched.emptySamples < kEmptySampleThreshold && ++a_latched.emptySamples == kEmptySampleThreshold)
		{
			a_latched.value.clear();
		}
	}

	LocationDetails LocationResolver::Resolve(RE::PlayerCharacter* a_player)
	{
		if (a_player)
		{
			Update(location_, ReadLocation(*a_player));
			Update(worldspace_, ReadWorldspace(*a_player));
		}
		else
		{
			Update(location_, {});
			Update(worldspace_, {});
		}

		LocationDetails result{ .location = location_.value, .worldspace = worldspace_.value };
		if (!result.worldspace.empty() && result.worldspace == result.location)
		{
			result.worldspace.clear();
		}

		return result;
	}
}
