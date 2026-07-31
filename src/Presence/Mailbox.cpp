#include "pch.h"

#include "Presence/Mailbox.h"

#include <mutex>
#include <utility>

namespace Presence
{
	void Mailbox::Publish(Activity a_activity)
	{
		{
			const std::scoped_lock lock{ mutex_ };
			if (stopped_)
			{
				return;
			}

			pending_ = std::move(a_activity);
			++published_;
		}

		ready_.notify_one();
	}

	bool Mailbox::Take(Activity& a_out)
	{
		std::unique_lock lock{ mutex_ };
		ready_.wait(lock, [this] { return stopped_ || published_ != taken_; });

		if (stopped_)
		{
			return false;
		}

		a_out = pending_;
		taken_ = published_;
		return true;
	}

	void Mailbox::Stop()
	{
		{
			const std::scoped_lock lock{ mutex_ };
			stopped_ = true;
		}

		ready_.notify_all();
	}
}
