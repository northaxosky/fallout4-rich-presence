#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Presence::Utf8Text
{
	struct DecodedCodePoint
	{
		std::uint32_t value;
		std::size_t   length;
		bool          valid;
	};

	[[nodiscard]] constexpr bool IsContinuationByte(unsigned char a_byte) noexcept
	{
		return (a_byte & 0xC0u) == 0x80u;
	}

	[[nodiscard]] inline DecodedCodePoint DecodeUtf8(std::string_view a_value, std::size_t a_position) noexcept
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

	struct SeparatorMetrics
	{
		std::size_t prefixLength;
		std::size_t suffixLength;
		bool        prefixHasPunctuation;
		bool        suffixHasPunctuation;
	};

	[[nodiscard]] inline bool HasPunctuation(std::string_view a_value) noexcept
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

	[[nodiscard]] inline SeparatorMetrics MeasureSeparators(std::string_view a_literal) noexcept
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

	inline void CollapseWhitespace(std::string& a_value)
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
