#include "pch.h"

#include "Presence/EngineText.h"
#include "Presence/Utf8Text.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	using namespace Presence::Utf8Text;

	[[nodiscard]] constexpr bool IsAsciiLetter(char a_value) noexcept
	{
		return (a_value >= 'A' && a_value <= 'Z') || (a_value >= 'a' && a_value <= 'z');
	}

	// the engine only substitutes '<' Key ['.' Field] '=' Value '>'
	[[nodiscard]] std::size_t TagLength(std::string_view a_text, std::size_t a_open) noexcept
	{
		if (a_open + 1 >= a_text.size() || !IsAsciiLetter(a_text[a_open + 1]))
		{
			return 0;
		}

		const auto close = a_text.find('>', a_open + 1);
		if (close == std::string_view::npos)
		{
			return 0;
		}

		const auto body = a_text.substr(a_open + 1, close - a_open - 1);
		return body.contains('=') && !body.contains('<') ? close - a_open + 1 : 0;
	}

	void TrimSeparators(std::string& a_value)
	{
		const auto metrics = MeasureSeparators(a_value);
		if (metrics.prefixLength == a_value.size())
		{
			a_value.clear();
			return;
		}

		a_value = a_value.substr(metrics.prefixLength, a_value.size() - metrics.prefixLength - metrics.suffixLength);
	}
}

namespace Presence
{
	void StripUnresolvedTags(std::string& a_text, std::string_view a_invalidTagPlaceholder)
	{
		const std::string_view text{ a_text };
		std::string            result;
		bool                   stripped = false;
		result.reserve(text.size());

		for (std::size_t position = 0; position < text.size();)
		{
			if (!a_invalidTagPlaceholder.empty() && text.substr(position).starts_with(a_invalidTagPlaceholder))
			{
				position += a_invalidTagPlaceholder.size();
				stripped = true;
				continue;
			}

			if (text[position] == '<')
			{
				if (const auto length = TagLength(text, position); length != 0)
				{
					position += length;
					stripped = true;
					continue;
				}
			}

			result.push_back(text[position++]);
		}

		if (!stripped)
		{
			return;
		}

		CollapseWhitespace(result);
		TrimSeparators(result);
		a_text = std::move(result);
	}
}
