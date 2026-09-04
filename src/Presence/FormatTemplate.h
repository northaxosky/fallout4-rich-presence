#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace Presence
{
	inline constexpr std::size_t kFormatTemplateSourceLimit = 512;

	enum class FormatToken : std::uint8_t
	{
		kName,
		kLevel,
		kQuest,
		kObjective,
		kLocation,
		kWorldspace,
		kState,
		kTarget
	};

	struct FormatValues
	{
		std::string_view name;
		std::string_view level;
		std::string_view quest;
		std::string_view objective;
		std::string_view location;
		std::string_view worldspace;
		std::string_view state;
		std::string_view target;
	};

	struct FormatTemplateError
	{
		std::size_t position;
		std::string message;
	};

	class FormatTemplate
	{
	public:
		FormatTemplate() = default;

		[[nodiscard]] static std::expected<FormatTemplate, FormatTemplateError> Compile(std::string_view a_source);

		[[nodiscard]] std::string Render(const FormatValues& a_values) const;

	private:
		enum class SegmentKind : std::uint8_t
		{
			kLiteral,
			kToken
		};

		struct Segment
		{
			SegmentKind kind;
			std::string literal;
			FormatToken token;
			std::size_t separatorPrefixLength;
			std::size_t separatorSuffixLength;
			bool        prefixHasPunctuation;
			bool        suffixHasPunctuation;
		};

		explicit FormatTemplate(std::vector<Segment> a_segments, bool a_hasTokens);

		std::vector<Segment> segments_;
		bool                 hasTokens_{ false };
	};
}
