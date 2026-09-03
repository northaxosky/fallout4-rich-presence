#include "pch.h"

#include "Game/LocationResolver.h"
#include "Game/MarkerCache.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
	inline constexpr int kMaxWorldspaceDepth = 32;

	[[nodiscard]] RE::MapMarkerData* ReadMarkerData(RE::TESObjectREFR* a_reference) noexcept
	{
		if (!a_reference || !a_reference->extraList)
		{
			return nullptr;
		}

		const auto marker = a_reference->extraList->GetByType<RE::ExtraMapMarker>();
		return marker ? marker->mapMarkerData : nullptr;
	}

	[[nodiscard]] bool IsWorldspaceOrAncestor(const RE::TESWorldSpace* a_candidate, const RE::TESWorldSpace* a_worldspace) noexcept
	{
		auto depth = 0;
		for (auto current = a_worldspace; current && depth < kMaxWorldspaceDepth; current = current->parentWorld)
		{
			if (current == a_candidate)
			{
				return true;
			}
			++depth;
		}
		return false;
	}

	[[nodiscard]] std::string ReadMarkerName(RE::MapMarkerData& a_marker)
	{
		const auto locationName = a_marker.GetLocationName();
		if (locationName && *locationName)
		{
			return locationName;
		}

		const auto fullName = a_marker.GetFullName();
		return fullName && *fullName ? std::string{ fullName } : std::string{};
	}
}

namespace Game
{
	void MarkerCache::Build()
	{
		const auto         started = std::chrono::steady_clock::now();
		std::vector<Entry> entries;

		if (const auto dataHandler = RE::TESDataHandler::GetSingleton())
		{
			for (const auto worldspace : dataHandler->GetFormArray<RE::TESWorldSpace>())
			{
				if (!worldspace || !worldspace->persistentCell)
				{
					continue;
				}

				worldspace->persistentCell->ForEachReference([&entries, worldspace](RE::TESObjectREFR* a_reference) {
					const auto marker = ReadMarkerData(a_reference);
					if (!marker ||
						std::to_underlying(marker->type.get()) >= std::to_underlying(RE::MARKER_TYPE::kCountTotal))
					{
						return RE::BSContainer::ForEachResult::kContinue;
					}

					entries.push_back(Entry{
						.reference = a_reference,
						.position = a_reference->GetPosition(),
						.worldspace = worldspace,
						.type = marker->type.get() });
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		}

		const auto entryCount = entries.size();
		{
			const std::scoped_lock lock{ mutex_ };
			entries_ = std::move(entries);
		}
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - started);
		REX::INFO("Cached {} map markers", entryCount);
		REX::DEBUG("Map marker cache built in {} ms", elapsed.count());
	}

	void MarkerCache::Invalidate()
	{
		const std::scoped_lock lock{ mutex_ };
		latched_.reset();
		pending_.reset();
		pendingSamples_ = 0;
	}

	std::optional<MarkerDetails> MarkerCache::FindNearest(RE::PlayerCharacter* a_player, float a_maxDistance) const
	{
		if (!a_player || a_maxDistance <= 0.0F)
		{
			return std::nullopt;
		}

		const auto worldspace = ReadWorldspace(*a_player);
		if (!worldspace)
		{
			return std::nullopt;
		}

		const auto   cell = a_player->parentCell;
		const auto   position = cell && cell->IsInterior() ? a_player->exteriorPosition : a_player->GetPosition();
		auto         nearestDistanceSquared = a_maxDistance * a_maxDistance;
		const Entry* nearest = nullptr;

		for (const auto& entry : entries_)
		{
			if (!IsWorldspaceOrAncestor(entry.worldspace, worldspace))
			{
				continue;
			}

			// distance first: reading discovery walks the ref's extra-data list, so do it only for candidates
			const auto distanceSquared = position.GetSquaredDistance(entry.position);
			if (distanceSquared > nearestDistanceSquared)
			{
				continue;
			}

			const auto marker = ReadMarkerData(entry.reference);
			if (!marker || !marker->IsDiscovered())
			{
				continue;
			}

			nearest = &entry;
			nearestDistanceSquared = distanceSquared;
		}

		if (!nearest)
		{
			return std::nullopt;
		}

		const auto marker = ReadMarkerData(nearest->reference);
		if (!marker)
		{
			return std::nullopt;
		}

		return MarkerDetails{
			.type = nearest->type,
			.name = ReadMarkerName(*marker)
		};
	}

	std::optional<MarkerDetails> MarkerCache::Resolve(RE::PlayerCharacter* a_player, float a_maxDistance)
	{
		const std::scoped_lock lock{ mutex_ };

		auto current = FindNearest(a_player, a_maxDistance);
		if (current == latched_)
		{
			pending_.reset();
			pendingSamples_ = 0;
			return latched_;
		}

		if (current != pending_)
		{
			pending_ = std::move(current);
			pendingSamples_ = 1;
			return latched_;
		}

		if (pendingSamples_ < kSettleSampleThreshold)
		{
			++pendingSamples_;
		}
		if (pendingSamples_ >= kSettleSampleThreshold)
		{
			latched_ = std::move(pending_);
			pending_.reset();
			pendingSamples_ = 0;
		}

		return latched_;
	}
}
