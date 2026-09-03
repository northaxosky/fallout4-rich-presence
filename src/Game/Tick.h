#pragma once

#include <REL/Version.h>

namespace Presence
{
	class Mailbox;
}

namespace Game::Tick
{
	[[nodiscard]] bool IsSupportedRuntime(const REL::Version& a_runtime) noexcept;

	bool Install(const REL::Version& a_runtime);

	[[nodiscard]] Presence::Mailbox& GetMailbox() noexcept;
	void                             BeginSession() noexcept;
	void                             BuildMarkerCache();
	void                             InvalidateCaches();
	void                             ResetElapsedEpoch() noexcept;
}
