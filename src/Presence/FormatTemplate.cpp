#include "pch.h"

#include "Presence/FormatTemplate.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	struct DecodedCodePoint
	{
		std::uint32_t value;
		std::size_t   length;
		bool          valid;
	};

	struct SeparatorMetrics
	{
		std::size_t prefixLength;
		std::size_t suffixLength;
		bool        prefixHasPunctuation;
		bool        suffixHasPunctuation;
	};

	[[nodiscard]] constexpr bool IsContinuationByte(unsigned char a_byte) noexcept
	{
		return (a_byte & 0xC0u) == 0x80u;
	}

	[[nodiscard]] DecodedCodePoint DecodeUtf8(std::string_view a_value, std::size_t a_position) noexcept
	{
		const auto first = static_cast<unsigned char>(a_value[a_position]);
		if (first < 0x80u)
		{
			return { first, 1, true };
		}

		std::size_t   length = 0;
		std::uint32_t codePoint = 0;
		std::uint32_t minimum = 0;
		if (first >= 0xC2u && first <= 0xDFu)
		{
			length = 2;
			codePoint = first & 0x1Fu;
			minimum = 0x80u;
		}
		else if (first >= 0xE0u && first <= 0xEFu)
		{
			length = 3;
			codePoint = first & 0x0Fu;
			minimum = 0x800u;
		}
		else if (first >= 0xF0u && first <= 0xF4u)
		{
			length = 4;
			codePoint = first & 0x07u;
			minimum = 0x10000u;
		}
		else
		{
			return { first, 1, false };
		}

		if (a_position + length > a_value.size())
		{
			return { first, 1, false };
		}

		for (std::size_t index = 1; index < length; ++index)
		{
			const auto byte = static_cast<unsigned char>(a_value[a_position + index]);
			if (!IsContinuationByte(byte))
			{
				return { first, 1, false };
			}
			codePoint = (codePoint << 6u) | (byte & 0x3Fu);
		}

		if (codePoint < minimum ||
			codePoint > 0x10FFFFu ||
			(codePoint >= 0xD800u && codePoint <= 0xDFFFu))
		{
			return { first, 1, false };
		}

		return { codePoint, length, true };
	}

	[[nodiscard]] constexpr bool IsAsciiSeparator(std::uint32_t a_codePoint) noexcept
	{
		switch (a_codePoint)
		{
			case ' ':
			case '\t':
			case '-':
			case ',':
			case ':':
			case ';':
			case '|':
			case '/':
			case '~':
				return true;
			default:
				return false;
		}
	}

	[[nodiscard]] constexpr bool IsSeparatorCodePoint(std::uint32_t a_codePoint) noexcept
	{
		return IsAsciiSeparator(a_codePoint) ||
		       a_codePoint == 0x00A0u ||
		       a_codePoint == 0x00B7u ||
		       (a_codePoint >= 0x2000u && a_codePoint <= 0x206Fu) ||
		       (a_codePoint >= 0x3000u && a_codePoint <= 0x303Fu) ||
		       (a_codePoint >= 0xFF00u && a_codePoint <= 0xFF65u);
	}

	[[nodiscard]] constexpr bool IsWhitespaceCodePoint(std::uint32_t a_codePoint) noexcept
	{
		return (a_codePoint <= 0x7Fu &&
				   (a_codePoint == ' ' ||
					   a_codePoint == '\t' ||
					   a_codePoint == '\n' ||
					   a_codePoint == '\r' ||
					   a_codePoint == '\f' ||
					   a_codePoint == '\v')) ||
		       a_codePoint == 0x00A0u ||
		       a_codePoint == 0x1680u ||
		       (a_codePoint >= 0x2000u && a_codePoint <= 0x200Au) ||
		       a_codePoint == 0x2028u ||
		       a_codePoint == 0x2029u ||
		       a_codePoint == 0x202Fu ||
		       a_codePoint == 0x205Fu ||
		       a_codePoint == 0x3000u;
	}

	[[nodiscard]] bool HasPunctuation(std::string_view a_value) noexcept
	{
		for (std::size_t position = 0; position < a_value.size();)
		{
			const auto decoded = DecodeUtf8(a_value, position);
			if (decoded.valid &&
				IsSeparatorCodePoint(decoded.value) &&
				!IsWhitespaceCodePoint(decoded.value))
			{
				return true;
			}
			position += decoded.length;
		}
		return false;
	}

	[[nodiscard]] SeparatorMetrics MeasureSeparators(std::string_view a_literal) noexcept
	{
		std::size_t prefixLength = 0;
		std::size_t suffixStart = a_literal.size();
		bool        suffixActive = false;

		for (std::size_t position = 0; position < a_literal.size();)
		{
			const auto decoded = DecodeUtf8(a_literal, position);
			const auto separator = decoded.valid && IsSeparatorCodePoint(decoded.value);
			if (position == prefixLength && separator)
			{
				prefixLength += decoded.length;
			}

			if (separator)
			{
				if (!suffixActive)
				{
					suffixStart = position;
					suffixActive = true;
				}
			}
			else
			{
				suffixStart = a_literal.size();
				suffixActive = false;
			}
			position += decoded.length;
		}

		const auto suffixLength = suffixActive ? a_literal.size() - suffixStart : 0;
		return {
			.prefixLength = prefixLength,
			.suffixLength = suffixLength,
			.prefixHasPunctuation = HasPunctuation(a_literal.substr(0, prefixLength)),
			.suffixHasPunctuation = HasPunctuation(a_literal.substr(a_literal.size() - suffixLength))
		};
	}

	[[nodiscard]] std::expected<Presence::FormatToken, Presence::FormatTemplateError> ParseToken(std::string_view a_token, std::size_t a_position)
	{
		using Presence::FormatToken;

		if (a_token == "name")
		{
			return FormatToken::kName;
		}
		if (a_token == "level")
		{
			return FormatToken::kLevel;
		}
		if (a_token == "quest")
		{
			return FormatToken::kQuest;
		}
		if (a_token == "objective")
		{
			return FormatToken::kObjective;
		}
		if (a_token == "location")
		{
			return FormatToken::kLocation;
		}
		if (a_token == "worldspace")
		{
			return FormatToken::kWorldspace;
		}
		if (a_token == "state")
		{
			return FormatToken::kState;
		}
		if (a_token == "target")
		{
			return FormatToken::kTarget;
		}

		return std::unexpected(Presence::FormatTemplateError{
			.position = a_position,
			.message = "unknown token {" + std::string{ a_token } + "}" });
	}

	[[nodiscard]] std::string_view ResolveToken(Presence::FormatToken a_token, const Presence::FormatValues& a_values) noexcept
	{
		switch (a_token)
		{
			case Presence::FormatToken::kName:
				return a_values.name;
			case Presence::FormatToken::kLevel:
				return a_values.level;
			case Presence::FormatToken::kQuest:
				return a_values.quest;
			case Presence::FormatToken::kObjective:
				return a_values.objective;
			case Presence::FormatToken::kLocation:
				return a_values.location;
			case Presence::FormatToken::kWorldspace:
				return a_values.worldspace;
			case Presence::FormatToken::kState:
				return a_values.state;
			case Presence::FormatToken::kTarget:
				return a_values.target;
		}

		return {};
	}

	void CollapseWhitespace(std::string& a_value)
	{
		std::size_t read = 0;
		std::size_t write = 0;
		bool        pendingSpace = false;

		while (read < a_value.size())
		{
			const auto decoded = DecodeUtf8(a_value, read);
			if (decoded.valid && IsWhitespaceCodePoint(decoded.value))
			{
				pendingSpace = write != 0;
				read += decoded.length;
				continue;
			}

			if (pendingSpace)
			{
				a_value[write++] = ' ';
				pendingSpace = false;
			}
			for (std::size_t index = 0; index < decoded.length; ++index)
			{
				a_value[write++] = a_value[read + index];
			}
			read += decoded.length;
		}

		a_value.resize(write);
	}
}

namespace Presence
{
	FormatTemplate::FormatTemplate(std::vector<Segment> a_segments, bool a_hasTokens) :
		segments_(std::move(a_segments)),
		hasTokens_(a_hasTokens)
	{}

	std::expected<FormatTemplate, FormatTemplateError> FormatTemplate::Compile(std::string_view a_source)
	{
		if (a_source.size() > kFormatTemplateSourceLimit)
		{
			return std::unexpected(FormatTemplateError{
				.position = kFormatTemplateSourceLimit,
				.message = "template exceeds 512 bytes" });
		}

		std::vector<Segment> segments;
		std::size_t          literalStart = 0;
		std::size_t          position = 0;
		bool                 hasTokens = false;

		const auto addLiteral = [&segments](std::string_view a_literal) {
			const auto metrics = MeasureSeparators(a_literal);
			segments.push_back(Segment{
				.kind = SegmentKind::kLiteral,
				.literal = std::string{ a_literal },
				.token = FormatToken::kName,
				.separatorPrefixLength = metrics.prefixLength,
				.separatorSuffixLength = metrics.suffixLength,
				.prefixHasPunctuation = metrics.prefixHasPunctuation,
				.suffixHasPunctuation = metrics.suffixHasPunctuation });
		};

		while (position < a_source.size())
		{
			if (a_source[position] == '}')
			{
				return std::unexpected(FormatTemplateError{
					.position = position,
					.message = "unmatched closing brace" });
			}

			if (a_source[position] != '{')
			{
				++position;
				continue;
			}

			if (position > literalStart)
			{
				addLiteral(a_source.substr(literalStart, position - literalStart));
			}

			const auto closingBrace = a_source.find('}', position + 1);
			if (closingBrace == std::string_view::npos)
			{
				return std::unexpected(FormatTemplateError{
					.position = position,
					.message = "unmatched opening brace" });
			}

			const auto tokenText = a_source.substr(position + 1, closingBrace - position - 1);
			if (tokenText.contains('{'))
			{
				return std::unexpected(FormatTemplateError{
					.position = position,
					.message = "nested opening brace" });
			}

			auto token = ParseToken(tokenText, position);
			if (!token)
			{
				return std::unexpected(std::move(token.error()));
			}
			segments.push_back(Segment{
				.kind = SegmentKind::kToken,
				.literal = {},
				.token = *token,
				.separatorPrefixLength = 0,
				.separatorSuffixLength = 0,
				.prefixHasPunctuation = false,
				.suffixHasPunctuation = false });
			hasTokens = true;

			position = closingBrace + 1;
			literalStart = position;
		}

		if (literalStart < a_source.size())
		{
			addLiteral(a_source.substr(literalStart));
		}

		return FormatTemplate{ std::move(segments), hasTokens };
	}

	std::string FormatTemplate::Render(const FormatValues& a_values) const
	{
		if (hasTokens_)
		{
			const auto hasResolvedToken = std::ranges::any_of(segments_, [&a_values](const auto& a_segment) {
				return a_segment.kind == SegmentKind::kToken && !ResolveToken(a_segment.token, a_values).empty();
			});
			if (!hasResolvedToken)
			{
				return {};
			}
		}

		std::size_t capacity = 0;
		for (const auto& segment : segments_)
		{
			capacity += segment.kind == SegmentKind::kLiteral ? segment.literal.size() : ResolveToken(segment.token, a_values).size();
		}

		std::string      output;
		std::string_view boundary;
		std::string_view lastLiteralSuffix;
		std::size_t      lastLiteralSuffixLength = 0;
		bool             boundaryHasPunctuation = false;
		bool             lastSuffixHasPunctuation = false;
		bool             pendingSeam = false;
		bool             lastWasLiteral = false;
		output.reserve(capacity);

		const auto mergeBoundary = [&boundary, &boundaryHasPunctuation](std::string_view a_candidate, bool a_hasPunctuation) {
			if (!a_candidate.empty() &&
				(boundary.empty() || (!boundaryHasPunctuation && a_hasPunctuation)))
			{
				boundary = a_candidate;
				boundaryHasPunctuation = a_hasPunctuation;
			}
		};
		const auto emitBoundary = [&output, &boundary, &boundaryHasPunctuation, &pendingSeam]() {
			if (!output.empty() && !boundary.empty())
			{
				output += boundary;
			}
			boundary = {};
			boundaryHasPunctuation = false;
			pendingSeam = false;
		};

		for (const auto& segment : segments_)
		{
			if (segment.kind == SegmentKind::kToken)
			{
				const auto value = ResolveToken(segment.token, a_values);
				if (value.empty())
				{
					if (lastWasLiteral && lastLiteralSuffixLength != 0)
					{
						output.resize(output.size() - lastLiteralSuffixLength);
						mergeBoundary(lastLiteralSuffix, lastSuffixHasPunctuation);
					}
					pendingSeam = true;
				}
				else
				{
					if (pendingSeam)
					{
						emitBoundary();
					}
					output += value;
				}

				lastLiteralSuffix = {};
				lastLiteralSuffixLength = 0;
				lastSuffixHasPunctuation = false;
				lastWasLiteral = false;
				continue;
			}

			auto literal = std::string_view{ segment.literal };
			if (pendingSeam)
			{
				mergeBoundary(literal.substr(0, segment.separatorPrefixLength), segment.prefixHasPunctuation);
				literal.remove_prefix(segment.separatorPrefixLength);
			}

			if (!literal.empty())
			{
				if (pendingSeam)
				{
					emitBoundary();
				}
				output += literal;
			}

			lastLiteralSuffixLength = std::min(segment.separatorSuffixLength, literal.size());
			lastLiteralSuffix = literal.substr(literal.size() - lastLiteralSuffixLength);
			lastSuffixHasPunctuation = segment.suffixHasPunctuation;
			lastWasLiteral = !literal.empty();
		}

		CollapseWhitespace(output);
		return output;
	}
}
