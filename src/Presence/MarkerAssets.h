#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Presence
{
	enum class MarkerType : std::uint32_t
	{
		kCave = 0x00,
		kCity = 0x01,
		kDiamondCity = 0x02,
		kIndustrial = 0x04,
		kMetro = 0x06,
		kMilitaryBase = 0x07,
		kSanctuary = 0x0C,
		kSettlement = 0x0D,
		kVault = 0x0F,
		kBunkerHill = 0x11,
		kChurch = 0x14,
		kDriveIn = 0x17,
		kFarm = 0x1A,
		kFillingStation = 0x1B,
		kGoodNeighbor = 0x1D,
		kGraveyard = 0x1E,
		kHospital = 0x1F,
		kInstitute = 0x22,
		kJunkyard = 0x24,
		kPondOrLake = 0x27,
		kRadioactiveArea = 0x29,
		kRadioTower = 0x2A,
		kSchool = 0x2C,
		kBOS = 0x32,
		kCastle = 0x35,
		kMinutemen = 0x39,
		kPoliceStation = 0x3A,
		kPrydwen = 0x3B,
		kRailroadFaction = 0x3C,
		kRailroad = 0x3D
	};

	struct MarkerAsset
	{
		MarkerType       type;
		std::string_view key;
	};

	inline constexpr std::array kMarkerAssets{
		MarkerAsset{ MarkerType::kVault, "marker_vault" },
		MarkerAsset{ MarkerType::kCity, "marker_city" },
		MarkerAsset{ MarkerType::kDiamondCity, "marker_diamondcity" },
		MarkerAsset{ MarkerType::kInstitute, "marker_institute" },
		MarkerAsset{ MarkerType::kBOS, "marker_bos" },
		MarkerAsset{ MarkerType::kPrydwen, "marker_prydwen" },
		MarkerAsset{ MarkerType::kMinutemen, "marker_minutemen" },
		MarkerAsset{ MarkerType::kCastle, "marker_castle" },
		MarkerAsset{ MarkerType::kRailroad, "marker_railroad" },
		MarkerAsset{ MarkerType::kRailroadFaction, "marker_railroad" },
		MarkerAsset{ MarkerType::kMetro, "marker_metro" },
		MarkerAsset{ MarkerType::kChurch, "marker_church" },
		MarkerAsset{ MarkerType::kFarm, "marker_farm" },
		MarkerAsset{ MarkerType::kSettlement, "marker_settlement" },
		MarkerAsset{ MarkerType::kSanctuary, "marker_sanctuary" },
		MarkerAsset{ MarkerType::kIndustrial, "marker_industrial" },
		MarkerAsset{ MarkerType::kMilitaryBase, "marker_militarybase" },
		MarkerAsset{ MarkerType::kHospital, "marker_hospital" },
		MarkerAsset{ MarkerType::kSchool, "marker_school" },
		MarkerAsset{ MarkerType::kCave, "marker_cave" },
		MarkerAsset{ MarkerType::kJunkyard, "marker_junkyard" },
		MarkerAsset{ MarkerType::kPondOrLake, "marker_water" },
		MarkerAsset{ MarkerType::kRadioactiveArea, "marker_radioactive" },
		MarkerAsset{ MarkerType::kRadioTower, "marker_radiotower" },
		MarkerAsset{ MarkerType::kPoliceStation, "marker_policestation" },
		MarkerAsset{ MarkerType::kGraveyard, "marker_graveyard" },
		MarkerAsset{ MarkerType::kFillingStation, "marker_fillingstation" },
		MarkerAsset{ MarkerType::kDriveIn, "marker_drivein" },
		MarkerAsset{ MarkerType::kGoodNeighbor, "marker_goodneighbor" },
		MarkerAsset{ MarkerType::kBunkerHill, "marker_bunkerhill" }
	};

	[[nodiscard]] std::string_view MarkerAssetKey(std::uint32_t a_markerType) noexcept;
}
