#include "pch.h"

#include "Presence/Mailbox.h"

#include <mutex>
#include <utility>

namespace Presence
{
	void Mailbox::Publish(ActivityUpdate a_update)
	{
		const std::scoped_lock lock{ mutex_ };
		pending_ = std::move(a_update);
		++published_;
	}

	bool Mailbox::TryTake(ActivityUpdate& a_out)
	{
		const std::scoped_lock lock{ mutex_ };
		if (published_ == taken_)
		{
			return false;
		}

		a_out = std::move(pending_);
		taken_ = published_;
		return true;
	}
}
