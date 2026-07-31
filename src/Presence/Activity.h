#pragma once

#include <cstdint>
#include <string>

namespace Presence
{
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

		[[nodiscard]] bool Empty() const noexcept { return details.empty() && state.empty(); }
	};
}
