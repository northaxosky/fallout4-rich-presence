#include "Presence/MenuActivity.h"
#include "Presence/StateBadge.h"

#include <array>
#include <iostream>
#include <string_view>

namespace
{
	struct LabelCase
	{
		Presence::MenuActivity activity;
		std::string_view       expected;
	};

	[[nodiscard]] bool CheckValue(
		std::string_view                                             a_test,
		const Presence::DebouncedValue<Presence::MenuActivityValue>& a_value,
		Presence::MenuActivity                                       a_expectedActivity,
		std::string_view                                             a_expectedName)
	{
		const auto& actual = a_value.Get();
		if (actual.activity != a_expectedActivity || actual.name != a_expectedName)
		{
			std::cerr << "FAIL " << a_test << ": expected " << Presence::ToString(a_expectedActivity)
					  << " \"" << a_expectedName << "\", got " << Presence::ToString(actual.activity)
					  << " \"" << actual.name << "\"\n";
			return false;
		}
		return true;
	}
}

int main()
{
	bool passed = true;

	const Presence::MenuActivityLabels labels{
		.barter = "Bartering",
		.workbench = "Crafting",
		.workshop = "Constructing",
		.terminal = "Computing",
		.lockpicking = "",
		.sitWait = "Resting",
		.dialogue = "Speaking"
	};
	constexpr std::array labelCases{
		LabelCase{ Presence::MenuActivity::kNone, "" },
		LabelCase{ Presence::MenuActivity::kBarter, "Bartering" },
		LabelCase{ Presence::MenuActivity::kWorkbench, "Crafting" },
		LabelCase{ Presence::MenuActivity::kWorkshop, "Constructing" },
		LabelCase{ Presence::MenuActivity::kTerminal, "Computing" },
		LabelCase{ Presence::MenuActivity::kLockpicking, "" },
		LabelCase{ Presence::MenuActivity::kSitWait, "Resting" },
		LabelCase{ Presence::MenuActivity::kDialogue, "Speaking" }
	};
	for (const auto& test : labelCases)
	{
		const auto actual = Presence::MenuActivityLabel(test.activity, labels);
		if (actual != test.expected)
		{
			std::cerr << "FAIL label " << Presence::ToString(test.activity) << ": expected \""
					  << test.expected << "\", got \"" << actual << "\"\n";
			passed = false;
		}
	}

	struct StringCase
	{
		Presence::MenuActivity activity;
		std::string_view       expected;
	};
	constexpr std::array stringCases{
		StringCase{ Presence::MenuActivity::kNone, "none" },
		StringCase{ Presence::MenuActivity::kBarter, "barter" },
		StringCase{ Presence::MenuActivity::kWorkbench, "workbench" },
		StringCase{ Presence::MenuActivity::kWorkshop, "workshop" },
		StringCase{ Presence::MenuActivity::kTerminal, "terminal" },
		StringCase{ Presence::MenuActivity::kLockpicking, "lockpicking" },
		StringCase{ Presence::MenuActivity::kSitWait, "sit_wait" },
		StringCase{ Presence::MenuActivity::kDialogue, "dialogue" }
	};
	for (const auto& test : stringCases)
	{
		const auto actual = Presence::ToString(test.activity);
		if (actual != test.expected)
		{
			std::cerr << "FAIL identifier: expected \"" << test.expected << "\", got \""
					  << actual << "\"\n";
			passed = false;
		}
	}

	Presence::DebouncedValue<Presence::MenuActivityValue> value;
	value.Update({ .activity = Presence::MenuActivity::kBarter, .name = "Trashcan Carla" });
	passed &= CheckValue("before threshold", value, Presence::MenuActivity::kNone, "");
	value.Update({ .activity = Presence::MenuActivity::kBarter, .name = "Trashcan Carla" });
	passed &= CheckValue("at threshold", value, Presence::MenuActivity::kBarter, "Trashcan Carla");
	value.Update({ .activity = Presence::MenuActivity::kWorkbench, .name = "Weapons Workbench" });
	passed &= CheckValue("previous persists", value, Presence::MenuActivity::kBarter, "Trashcan Carla");
	value.Update({ .activity = Presence::MenuActivity::kWorkbench, .name = "Weapons Workbench" });
	passed &= CheckValue("new activity publishes", value, Presence::MenuActivity::kWorkbench, "Weapons Workbench");
	value.Update({ .activity = Presence::MenuActivity::kWorkbench, .name = "Armor Workbench" });
	passed &= CheckValue("same id new name settles", value, Presence::MenuActivity::kWorkbench, "Weapons Workbench");
	value.Update({ .activity = Presence::MenuActivity::kWorkbench, .name = "Armor Workbench" });
	passed &= CheckValue("same id new name publishes", value, Presence::MenuActivity::kWorkbench, "Armor Workbench");

	if (passed)
	{
		std::cout << "ALL TESTS PASSED\n";
	}
	return passed ? 0 : 1;
}
