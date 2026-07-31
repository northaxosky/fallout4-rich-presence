#pragma once

#include "Presence/Activity.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace Presence
{
	// single slot latest-wins handoff
	class Mailbox
	{
	public:
		void Publish(Activity a_activity);

		// blocks until a newer activity arrives or Stop() is called
		[[nodiscard]] bool Take(Activity& a_out);

		void Stop();

	private:
		std::mutex              mutex_;
		std::condition_variable ready_;
		Activity                pending_;
		std::uint64_t           published_{ 0 };
		std::uint64_t           taken_{ 0 };
		bool                    stopped_{ false };
	};
}
