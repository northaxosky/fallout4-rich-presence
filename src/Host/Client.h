#pragma once

#include "Game/Conflicts.h"

#include <vector>

namespace Host
{
	void SetConflicts(std::vector<Game::Conflict> a_conflicts) noexcept;
	void Connect() noexcept;
}
