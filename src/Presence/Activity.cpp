#include "pch.h"

#include "Presence/Activity.h"

#include <string>
#include <string_view>
#include <utility>

namespace
{
	[[nodiscard]] std::string TruncateUtf8(std::string_view a_value, std::size_t a_limit)
	{
		if (a_value.size() <= a_limit)
		{
			return std::string{ a_value };
		}

		auto size = a_limit;
		while (size > 0 && (static_cast<unsigned char>(a_value[size]) & 0xC0u) == 0x80u)
		{
			--size;
		}
		return std::string{ a_value.substr(0, size) };
	}
}

namespace Presence
{
	Activity NormalizeActivity(Activity a_activity)
	{
		a_activity.details = TruncateUtf8(a_activity.details, kActivityTextLimit);
		a_activity.state = TruncateUtf8(a_activity.state, kActivityTextLimit);
		a_activity.largeImage = TruncateUtf8(a_activity.largeImage, kActivityAssetKeyLimit);
		a_activity.largeText = TruncateUtf8(a_activity.largeText, kActivityTextLimit);
		a_activity.smallImage = TruncateUtf8(a_activity.smallImage, kActivityAssetKeyLimit);
		a_activity.smallText = TruncateUtf8(a_activity.smallText, kActivityTextLimit);
		return a_activity;
	}

	ActivityUpdate NormalizeActivity(ActivityUpdate a_update)
	{
		if (a_update)
		{
			*a_update = NormalizeActivity(std::move(*a_update));
		}
		return a_update;
	}
}
