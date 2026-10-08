#pragma once

#include <string>
#include <string_view>

namespace Presence
{
	// normalizes the text only when markup was actually removed
	void StripUnresolvedTags(std::string& a_text, std::string_view a_invalidTagPlaceholder);
}
