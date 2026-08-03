#include "pch.h"

#include "Presence/FormatTemplate.h"

#include <array>
#include <cctype>
#include <clocale>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
	class LocaleGuard
	{
	public:
		LocaleGuard()
		{
			if (const auto current = std::setlocale(LC_CTYPE, nullptr))
			{
				previous_ = current;
			}
		}

		~LocaleGuard()
		{
			if (!previous_.empty())
			{
				std::setlocale(LC_CTYPE, previous_.c_str());
			}
		}

		[[nodiscard]] bool ActivateSingleByteLocale()
		{
			constexpr std::array candidates{
				"English_United States.1252",
				".1252",
				"Russian_Russia.1251"
			};

			for (const auto candidate : candidates)
			{
				if (std::setlocale(LC_CTYPE, candidate) &&
					std::isspace(static_cast<unsigned char>(0xA0u)) != 0)
				{
					return true;
				}
			}
			return false;
		}

	private:
		std::string previous_;
	};

	[[nodiscard]] bool Check(std::string_view a_name, std::string_view a_source, const Presence::FormatValues& a_values, std::string_view a_expected)
	{
		const auto format = Presence::FormatTemplate::Compile(a_source);
		if (!format)
		{
			std::cerr << "FAIL " << a_name << ": compile error " << format.error().message << '\n';
			return false;
		}

		const auto actual = format->Render(a_values);
		if (actual != a_expected)
		{
			std::cerr << "FAIL " << a_name << ": expected \"" << a_expected << "\", got \"" << actual << "\"\n";
			return false;
		}

		std::cout << "PASS " << a_name << " -> \"" << actual << "\"\n";
		return true;
	}

	[[nodiscard]] bool Rejects(std::string_view a_name, std::string_view a_source)
	{
		const auto format = Presence::FormatTemplate::Compile(a_source);
		if (format)
		{
			std::cerr << "FAIL " << a_name << ": invalid template accepted\n";
			return false;
		}

		std::cout << "PASS " << a_name << " rejected -> " << format.error().message << '\n';
		return true;
	}

	[[nodiscard]] bool Compiles(std::string_view a_name, std::string_view a_source)
	{
		if (!Presence::FormatTemplate::Compile(a_source))
		{
			std::cerr << "FAIL " << a_name << ": valid template rejected\n";
			return false;
		}

		std::cout << "PASS " << a_name << " compiled\n";
		return true;
	}
}

int main()
{
	Presence::FormatValues emptyValues{};

	auto levelOnly = emptyValues;
	levelOnly.level = "12";

	auto questOnly = emptyValues;
	questOnly.quest = "Reunions";

	auto nameOnly = emptyValues;
	nameOnly.name = "Sole Survivor";

	auto questAndLocation = questOnly;
	questAndLocation.location = "Diamond City";

	auto worldspaceOnly = emptyValues;
	worldspaceOnly.worldspace = "Commonwealth";

	auto levelAndWorldspace = levelOnly;
	levelAndWorldspace.worldspace = "Commonwealth";

	auto fullValues = levelAndWorldspace;
	fullValues.name = "Sole Survivor";
	fullValues.quest = "Reunions";
	fullValues.objective = "Find Nick Valentine";
	fullValues.location = "Diamond City";
	fullValues.state = "In Game";

	bool passed = true;
	passed &= Check("middle empty seam", "{quest} - {objective} - {location}", questAndLocation, "Reunions - Diamond City");
	passed &= Check("empty name between words", "Playing as {name} in {worldspace}", worldspaceOnly, "Playing as in Commonwealth");
	passed &= Check("empty location between words", "Level {level} in {location} now", levelOnly, "Level 12 in now");
	passed &= Check("closing bracket preserved", "[{name}] {quest}", nameOnly, "[Sole Survivor]");
	passed &= Check("C++ all tokens empty", "C++ {name}", emptyValues, "");
	passed &= Check("C++ seam preserved", "C++ {name} {level}", levelOnly, "C++ 12");
	passed &= Check("non-separator punctuation", "[]()\"'+*#!?.@&=%$ {name} {level}", levelOnly, "[]()\"'+*#!?.@&=%$ 12");
	passed &= Check("empty name before label", "{name} - Level {level}", levelOnly, "Level 12");
	passed &= Check("empty trailing objective", "{quest}: {objective}", questOnly, "Reunions");
	passed &= Check("empty leading name", "{name} - {level}", levelOnly, "12");
	passed &= Check("populated separators", "{quest}: {objective}", fullValues, "Reunions: Find Nick Valentine");
	passed &= Check("all tokens empty label", "Quest: {quest}", emptyValues, "");
	passed &= Check("all tokens empty default", "{name} - Level {level}", emptyValues, "");
	passed &= Check("pure literal", "Constant presence", emptyValues, "Constant presence");
	passed &= Check("pure C++ literal", "C++", emptyValues, "C++");
	passed &= Check("Unicode en dash", "{name} – {level}", levelOnly, "12");
	passed &= Check("Unicode fullwidth colon", "{quest}：{objective}", questOnly, "Reunions");
	passed &= Check("Unicode CJK comma", "{quest}、{objective}", questOnly, "Reunions");
	passed &= Check("Unicode middle dot", "{name} · {level}", levelOnly, "12");
	passed &= Check("Unicode nonbreaking seam", "{name} {level}", levelOnly, "12");
	passed &= Check("ideographic whitespace", "　", emptyValues, "");
	passed &= Check("UTF-8 words preserved", "世界{name}{level}", levelOnly, "世界12");
	passed &= Rejects("unknown token", "{unknown}");
	passed &= Rejects("unmatched opening brace", "{name");
	passed &= Rejects("unmatched closing brace", "name}");
	passed &= Compiles("source length limit", std::string(Presence::kFormatTemplateSourceLimit, 'x'));
	passed &= Rejects("source over length limit", std::string(Presence::kFormatTemplateSourceLimit + 1, 'x'));

	{
		LocaleGuard locale;
		if (!locale.ActivateSingleByteLocale())
		{
			std::cerr << "FAIL non-C locale: no locale classifies 0xA0 as whitespace\n";
			passed = false;
		}
		else
		{
			const auto source = std::string{ "Caf\xC3\xA0 " } + "{level}";
			const auto expected = std::string{ "Caf\xC3\xA0 " } + "12";
			passed &= Check("non-C locale UTF-8", source, levelOnly, expected);
		}
	}

	if (passed)
	{
		std::cout << "ALL TESTS PASSED\n";
	}
	return passed ? 0 : 1;
}
