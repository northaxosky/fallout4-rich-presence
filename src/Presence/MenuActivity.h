#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Presence
{
	enum class MenuActivity : std::uint8_t
	{
		kNone,
		kBarter,
		kWorkbench,
		kWorkshop,
		kTerminal,
		kLockpicking,
		kSitWait,
		kDialogue
	};

	struct MenuActivityLabels
	{
		std::string_view barter;
		std::string_view workbench;
		std::string_view workshop;
		std::string_view terminal;
		std::string_view lockpicking;
		std::string_view sitWait;
		std::string_view dialogue;
	};

	struct MenuActivityValue
	{
		MenuActivity activity{ MenuActivity::kNone };
		std::string  name;

		bool operator==(const MenuActivityValue&) const = default;
	};

	[[nodiscard]] inline std::string_view MenuActivityLabel(
		MenuActivity              a_activity,
		const MenuActivityLabels& a_labels) noexcept
	{
		switch (a_activity)
		{
			case MenuActivity::kBarter:
				return a_labels.barter;
			case MenuActivity::kWorkbench:
				return a_labels.workbench;
			case MenuActivity::kWorkshop:
				return a_labels.workshop;
			case MenuActivity::kTerminal:
				return a_labels.terminal;
			case MenuActivity::kLockpicking:
				return a_labels.lockpicking;
			case MenuActivity::kSitWait:
				return a_labels.sitWait;
			case MenuActivity::kDialogue:
				return a_labels.dialogue;
			case MenuActivity::kNone:
				return {};
		}

		return {};
	}

	[[nodiscard]] constexpr std::string_view ToString(MenuActivity a_activity) noexcept
	{
		switch (a_activity)
		{
			case MenuActivity::kNone:
				return "none";
			case MenuActivity::kBarter:
				return "barter";
			case MenuActivity::kWorkbench:
				return "workbench";
			case MenuActivity::kWorkshop:
				return "workshop";
			case MenuActivity::kTerminal:
				return "terminal";
			case MenuActivity::kLockpicking:
				return "lockpicking";
			case MenuActivity::kSitWait:
				return "sit_wait";
			case MenuActivity::kDialogue:
				return "dialogue";
		}

		return "none";
	}
}
