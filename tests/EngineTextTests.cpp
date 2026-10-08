#include "pch.h"

#include "Presence/EngineText.h"

#include <iostream>
#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] bool Check(std::string_view a_name, std::string_view a_input, std::string_view a_expected)
	{
		std::string actual{ a_input };
		Presence::StripUnresolvedTags(actual, "[...]");
		if (actual != a_expected)
		{
			std::cerr << "FAIL " << a_name << ": expected \"" << a_expected << "\", got \"" << actual << "\"\n";
			return false;
		}

		std::cout << "PASS " << a_name << " -> \"" << actual << "\"\n";
		return true;
	}
}

int main()
{
	bool passed = true;
	passed &= Check("unresolved alias leaves no dangling separator", "Weathervane: <Alias=QuestLocation>", "Weathervane");
	passed &= Check("engine placeholder inside a sentence", "Clear [...] of raiders", "Clear of raiders");
	passed &= Check("field tag with capitalization suffix", "<Alias.ShortNameCap=Settler> needs help", "needs help");
	passed &= Check("multibyte separator trimmed whole", "Sanctuary \xE2\x80\x93 <Global=Count>", "Sanctuary");
	passed &= Check("non-tag angle text untouched", "Report to <Preston>  a < b", "Report to <Preston>  a < b");
	passed &= Check("only markup becomes empty", "<Alias=QuestLocation>", "");
	return passed ? 0 : 1;
}
