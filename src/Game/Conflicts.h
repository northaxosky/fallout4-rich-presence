#pragma once

#include <string>
#include <vector>

namespace Game
{
	struct Conflict
	{
		std::string module;
		std::string displayName;
	};

	[[nodiscard]] std::vector<Conflict> DetectConflicts();
}
