#pragma once

#include "Presence/Activity.h"

#include <cstdint>
#include <mutex>

namespace Presence
{
	// single slot latest-wins handoff
	class Mailbox
	{
	public:
		void Publish(ActivityUpdate a_update);

		[[nodiscard]] bool TryTake(ActivityUpdate& a_out);

	private:
		std::mutex     mutex_;
		ActivityUpdate pending_;
		std::uint64_t  published_{ 0 };
		std::uint64_t  taken_{ 0 };
	};
}
