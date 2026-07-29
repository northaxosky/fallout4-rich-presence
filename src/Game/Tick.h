#pragma once

#include <REL/Version.h>

namespace Game::Tick
{
	[[nodiscard]] bool IsSupportedRuntime(const REL::Version& a_runtime) noexcept;

	bool Install(const REL::Version& a_runtime);
}
