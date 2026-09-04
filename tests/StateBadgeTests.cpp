#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"
#include "Presence/StateBadge.h"

#include <array>
#include <iostream>
#include <string_view>

namespace
{
	[[nodiscard]] bool CheckBadge(
		std::string_view     a_name,
		bool                 a_combat,
		bool                 a_powerArmor,
		bool                 a_irradiated,
		Presence::StateBadge a_expected)
	{
		const auto actual = Presence::ResolveStateBadge(a_combat, a_powerArmor, a_irradiated);
		if (actual != a_expected)
		{
			std::cerr << "FAIL " << a_name << ": expected " << Presence::ToString(a_expected)
					  << ", got " << Presence::ToString(actual) << '\n';
			return false;
		}
		return true;
	}

	[[nodiscard]] bool CheckFlag(
		std::string_view               a_name,
		const Presence::DebouncedFlag& a_flag,
		bool                           a_expected)
	{
		if (a_flag.IsActive() != a_expected)
		{
			std::cerr << "FAIL " << a_name << ": expected " << a_expected
					  << ", got " << a_flag.IsActive() << '\n';
			return false;
		}
		return true;
	}
}

int main()
{
	bool passed = true;

	passed &= CheckBadge("none", false, false, false, Presence::StateBadge::kNone);
	passed &= CheckBadge("irradiated", false, false, true, Presence::StateBadge::kIrradiated);
	passed &= CheckBadge("power armor priority", false, true, true, Presence::StateBadge::kPowerArmor);
	passed &= CheckBadge("combat priority", true, true, true, Presence::StateBadge::kCombat);

	Presence::DebouncedFlag flag;
	flag.Update(true);
	passed &= CheckFlag("single differing sample", flag, false);
	flag.Update(false);
	passed &= CheckFlag("matching sample resets transition", flag, false);
	flag.Update(true);
	flag.Update(true);
	passed &= CheckFlag("threshold activates", flag, true);
	flag.Update(false);
	passed &= CheckFlag("single differing active sample", flag, true);
	flag.Update(true);
	flag.Update(false);
	flag.Update(false);
	passed &= CheckFlag("threshold deactivates", flag, false);
	flag.Update(true);
	flag.Update(true);
	flag.Reset();
	passed &= CheckFlag("reset", flag, false);

	const auto stateFormat = Presence::FormatTemplate::Compile("{state}");
	if (!stateFormat)
	{
		std::cerr << "FAIL state format did not compile\n";
		passed = false;
	}
	else
	{
		constexpr std::array badges{
			Presence::StateBadge::kNone,
			Presence::StateBadge::kCombat,
			Presence::StateBadge::kPowerArmor,
			Presence::StateBadge::kIrradiated
		};
		for (const auto badge : badges)
		{
			const auto label = Presence::StateBadgeLabel(badge);
			const auto rendered = stateFormat->Render(Presence::FormatValues{ .state = label });
			if (rendered != label)
			{
				std::cerr << "FAIL state label " << Presence::ToString(badge) << ": expected \""
						  << label << "\", got \"" << rendered << "\"\n";
				passed = false;
			}
		}
	}

	for (const auto asset : { Presence::Asset::kPowerArmor, Presence::Asset::kIrradiated })
	{
		const auto key = Presence::DefaultAssetKey(asset);
		if (!Presence::IsValidAssetKey(key))
		{
			std::cerr << "FAIL invalid default asset key \"" << key << "\"\n";
			passed = false;
		}
	}

	if (passed)
	{
		std::cout << "ALL TESTS PASSED\n";
	}
	return passed ? 0 : 1;
}
