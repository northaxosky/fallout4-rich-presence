#include "Presence/AssetKeys.h"
#include "Presence/FormatTemplate.h"
#include "Presence/StateBadge.h"

#include <array>
#include <iostream>
#include <string>
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

	struct Target
	{
		std::uint32_t id{ 0 };
		std::string   name;

		[[nodiscard]] bool operator==(const Target& a_rhs) const noexcept
		{
			return id == a_rhs.id;
		}
	};

	[[nodiscard]] bool CheckTarget(
		std::string_view                        a_test,
		const Presence::DebouncedValue<Target>& a_target,
		std::uint32_t                           a_expectedID,
		std::string_view                        a_expectedName)
	{
		const auto& actual = a_target.Get();
		if (actual.id != a_expectedID || actual.name != a_expectedName)
		{
			std::cerr << "FAIL " << a_test << ": expected " << a_expectedID << " \"" << a_expectedName
					  << "\", got " << actual.id << " \"" << actual.name << "\"\n";
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

	Presence::DebouncedValue<Target> target;
	target.Update(Target{ .id = 1, .name = "Raider" });
	passed &= CheckTarget("new target before threshold", target, 0, "");
	target.Update(Target{ .id = 1, .name = "Raider" });
	passed &= CheckTarget("new target at threshold", target, 1, "Raider");
	target.Update(Target{ .id = 2, .name = "Deathclaw" });
	passed &= CheckTarget("previous target while settling", target, 1, "Raider");
	target.Update(Target{ .id = 3, .name = "Mirelurk" });
	target.Update(Target{ .id = 2, .name = "Deathclaw" });
	passed &= CheckTarget("interrupted run restarts", target, 1, "Raider");
	target.Update(Target{ .id = 2, .name = "Deathclaw" });
	passed &= CheckTarget("restarted run publishes", target, 2, "Deathclaw");
	target.Update(Target{ .id = 3, .name = "Deathclaw" });
	passed &= CheckTarget("same name different ID settles", target, 2, "Deathclaw");
	target.Update(Target{ .id = 3, .name = "Deathclaw" });
	passed &= CheckTarget("same name different ID publishes", target, 3, "Deathclaw");
	target.Update(Target{ .id = 4, .name = "Gunner" });
	target.Reset();
	passed &= CheckTarget("value reset", target, 0, "");

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
