#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace Presence
{
	inline constexpr std::size_t kActivityTextLimit = 128;
	inline constexpr std::size_t kActivityAssetKeyLimit = 32;

	struct Activity
	{
		std::string  details;
		std::string  state;
		std::string  largeImage;
		std::string  largeText;
		std::string  smallImage;
		std::string  smallText;
		std::int64_t startTimestamp{ 0 };

		[[nodiscard]] bool operator==(const Activity& a_rhs) const noexcept
		{
			return details == a_rhs.details &&
			       state == a_rhs.state &&
			       largeImage == a_rhs.largeImage &&
			       largeText == a_rhs.largeText &&
			       smallImage == a_rhs.smallImage &&
			       smallText == a_rhs.smallText;
		}
	};

	// nullopt clears presence; an activity may legitimately contain only an icon or timer
	using ActivityUpdate = std::optional<Activity>;

	[[nodiscard]] Activity       NormalizeActivity(Activity a_activity);
	[[nodiscard]] ActivityUpdate NormalizeActivity(ActivityUpdate a_update);
}
