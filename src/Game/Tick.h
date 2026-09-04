#pragma once

#include <REL/Version.h>

#include <cstdint>

namespace Presence
{
	class Mailbox;
}

namespace Game::Tick
{
	[[nodiscard]] bool           IsSupportedRuntime(const REL::Version& a_runtime) noexcept;
	[[nodiscard]] bool           HasAddressLibrary(const REL::Version& a_runtime);
	[[nodiscard]] std::uintptr_t GetMainOnIdleAddress();

	bool Install(const REL::Version& a_runtime);

	[[nodiscard]] Presence::Mailbox& GetMailbox() noexcept;
	void                             BeginSession() noexcept;
	void                             BuildMarkerCache();
	void                             InvalidateCaches();
	void                             ResetElapsedEpoch() noexcept;
}
