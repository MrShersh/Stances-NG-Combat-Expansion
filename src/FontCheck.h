#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Which characters FLICK's current font can draw. Standard library only, so tools/fonttest can check it offline.
namespace FontCheck
{
	// FLICK's built-in "Default" typeface is Futura Std with 261 glyphs, Latin-1 only (unpacked from FLICK 1.5.0's
	// imgui_default.h and checked: no Cyrillic).
	inline constexpr char32_t kDefaultFontLast = 0xFF;

	std::vector<char32_t> DecodeUtf8(std::string_view a_text);
	// [Style] sFont of FLICK's defaultstyle.ini (a file name such as "Jost-Regular.ttf"), "Default" when unset.
	std::string FlickFontName(const std::filesystem::path& a_styleIni);
	// Looks the font up in FLICK's font folders in FLICK's order.
	std::optional<std::filesystem::path> FindFont(const std::string& a_name);
	// The subset of a_wanted that the font's Unicode cmap (format 4 or 12) maps to a glyph.
	std::set<char32_t> FontCharacters(const std::filesystem::path& a_font, const std::set<char32_t>& a_wanted);
}
