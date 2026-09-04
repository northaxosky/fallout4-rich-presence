#pragma once

#include <REL/Version.h>

#include <cstddef>
#include <cstdint>

namespace Presence
{
	struct Activity;
	class Mailbox;
}

namespace Game::Tick
{
	[[nodiscard]] bool           IsSupportedRuntime(const REL::Version& a_runtime) noexcept;
	[[nodiscard]] bool           HasAddressLibrary(const REL::Version& a_runtime);
	[[nodiscard]] std::uintptr_t GetMainOnIdleAddress();

	bool Install(const REL::Version& a_runtime);

	[[nodiscard]] Presence::Mailbox& GetMailbox() noexcept;
	[[nodiscard]] Presence::Activity GetPublishedActivity();
	[[nodiscard]] std::size_t        GetMarkerCount();
	void                             BeginSession() noexcept;
	void                             BuildMarkerCache();
	void                             InvalidateCaches();
	void                             ResetElapsedEpoch() noexcept;
}
