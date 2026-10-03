// Offline check of the font coverage test the mod uses to choose its UI language.
// Build: cl /std:c++latest /EHsc /I..\..\src main.cpp ..\..\src\FontCheck.cpp
// Usage: fonttest <font.ttf>...   prints which of the Russian letters and a few Latin-1 characters each font maps.
#include "FontCheck.h"

#include <cstdio>

int main(int argc, char** argv)
{
	std::set<char32_t> wanted{ U'é', U'ü' };
	for (char32_t cp = 0x410; cp <= 0x44F; ++cp) {
		wanted.insert(cp);
	}
	wanted.insert(0x401);
	wanted.insert(0x451);

	for (int i = 1; i < argc; ++i) {
		const auto found = FontCheck::FontCharacters(argv[i], wanted);
		std::size_t cyrillic = 0;
		for (const auto cp : found) {
			cyrillic += cp >= 0x400 && cp <= 0x4FF;
		}
		std::printf("%s: %zu of %zu, Cyrillic %zu of 66, e-acute %s\n", argv[i], found.size(), wanted.size(), cyrillic,
			found.contains(U'é') ? "yes" : "no");
	}
}
