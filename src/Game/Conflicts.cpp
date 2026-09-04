#include "pch.h"

#include "Game/Conflicts.h"

#include <REX/W32/KERNEL32.h>

#include <array>
#include <string_view>
#include <vector>

namespace
{
	struct KnownConflict
	{
		std::wstring_view module;
		std::string_view  moduleNarrow;
		std::string_view  displayName;
	};

	inline constexpr std::array kKnownConflicts{
		KnownConflict{
			.module = L"Discord_Presence_F4SE_Remake.dll",
			.moduleNarrow = "Discord_Presence_F4SE_Remake.dll",
			.displayName = "Discord Rich Presence REMAKE (F4SE)" },
		KnownConflict{
			.module = L"Discord_Presence_F4SE.dll",
			.moduleNarrow = "Discord_Presence_F4SE.dll",
			.displayName = "Discord Rich Presence for Fallout 4" }
	};
}

namespace Game
{
	std::vector<Conflict> DetectConflicts()
	{
		std::vector<Conflict> conflicts;
		for (const auto& known : kKnownConflicts)
		{
			if (REX::W32::GetModuleHandleW(known.module.data()))
			{
				conflicts.push_back(Conflict{
					.module = std::string{ known.moduleNarrow },
					.displayName = std::string{ known.displayName } });
			}
		}
		return conflicts;
	}
}
