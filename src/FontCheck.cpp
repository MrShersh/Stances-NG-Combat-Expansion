#include "FontCheck.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iterator>

namespace FontCheck
{
	namespace
	{
		namespace fs = std::filesystem;

		// FLICK System/Settings.h and ImGui/IconsFonts.cpp: user fonts, legacy ImGui Icons fonts, Community Shaders.
		constexpr std::array kFlickFontDirs{ "Data/FUCKs/FUCK/fonts", "Data/Interface/ImGuiIcons/Fonts",
			"Data/Interface/CommunityShaders/Fonts" };

		std::uint16_t BE16(const std::vector<std::uint8_t>& a_d, std::size_t a_o)
		{
			return a_o + 2 <= a_d.size() ? static_cast<std::uint16_t>(a_d[a_o] << 8 | a_d[a_o + 1]) : 0;
		}

		std::uint32_t BE32(const std::vector<std::uint8_t>& a_d, std::size_t a_o)
		{
			return static_cast<std::uint32_t>(BE16(a_d, a_o)) << 16 | BE16(a_d, a_o + 2);
		}

		bool HasFormat12(const std::vector<std::uint8_t>& a_d, std::size_t a_table, char32_t a_cp)
		{
			for (std::uint32_t g = 0, n = BE32(a_d, a_table + 12); g < n; ++g) {
				const auto group = a_table + 16 + static_cast<std::size_t>(g) * 12;
				if (a_cp >= BE32(a_d, group) && a_cp <= BE32(a_d, group + 4)) {
					return true;
				}
			}
			return false;
		}

		bool HasFormat4(const std::vector<std::uint8_t>& a_d, std::size_t a_table, char32_t a_cp)
		{
			if (a_cp > 0xFFFF) {
				return false;
			}
			const std::size_t segX2 = BE16(a_d, a_table + 6);
			const std::size_t ends = a_table + 14;
			const std::size_t starts = ends + segX2 + 2;
			const std::size_t deltas = starts + segX2;
			const std::size_t ranges = deltas + segX2;
			for (std::size_t s = 0; s < segX2 / 2; ++s) {
				if (a_cp > BE16(a_d, ends + s * 2)) {
					continue;
				}
				const auto start = BE16(a_d, starts + s * 2);
				if (a_cp < start) {
					return false;
				}
				const auto delta = BE16(a_d, deltas + s * 2);
				const auto rangeOffset = BE16(a_d, ranges + s * 2);
				if (rangeOffset == 0) {
					return static_cast<std::uint16_t>(a_cp + delta) != 0;
				}
				const auto glyph = BE16(a_d, ranges + s * 2 + rangeOffset + (a_cp - start) * 2);
				return glyph != 0 && static_cast<std::uint16_t>(glyph + delta) != 0;
			}
			return false;
		}
	}

	std::vector<char32_t> DecodeUtf8(std::string_view a_text)
	{
		std::vector<char32_t> result;
		for (std::size_t i = 0; i < a_text.size();) {
			const auto c = static_cast<unsigned char>(a_text[i]);
			const std::size_t length = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
			char32_t cp = length == 1 ? c : c & (0x3F >> (length - 1));
			for (std::size_t k = 1; k < length && i + k < a_text.size(); ++k) {
				cp = cp << 6 | (static_cast<unsigned char>(a_text[i + k]) & 0x3F);
			}
			result.push_back(cp);
			i += length;
		}
		return result;
	}

	std::string FlickFontName(const fs::path& a_styleIni)
	{
		std::ifstream ini(a_styleIni);
		bool          inStyle = false;
		for (std::string line; std::getline(ini, line);) {
			const auto first = line.find_first_not_of(" \t");
			if (first == std::string::npos) {
				continue;
			}
			line = line.substr(first);
			if (line.front() == '[') {
				inStyle = line.starts_with("[Style]");
			} else if (inStyle && line.starts_with("sFont")) {
				const auto eq = line.find('=');
				auto value = eq == std::string::npos ? std::string{} : line.substr(eq + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				value.erase(value.find_last_not_of(" \t\r") + 1);
				return value.empty() ? "Default" : value;
			}
		}
		return "Default";
	}

	std::optional<fs::path> FindFont(const std::string& a_name)
	{
		std::error_code ec;
		for (const auto dir : kFlickFontDirs) {
			if (!fs::exists(dir, ec)) {
				continue;
			}
			for (fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
				if (it->is_regular_file(ec) && it->path().filename().string() == a_name) {
					return it->path();
				}
			}
		}
		return std::nullopt;
	}

	std::set<char32_t> FontCharacters(const fs::path& a_font, const std::set<char32_t>& a_wanted)
	{
		std::set<char32_t> found;
		std::ifstream file(a_font, std::ios::binary);
		const std::vector<std::uint8_t> d{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

		std::size_t cmap = 0;
		for (std::uint16_t t = 0, n = BE16(d, 4); t < n; ++t) {
			const std::size_t record = 12 + static_cast<std::size_t>(t) * 16;
			if (record + 16 <= d.size() && std::memcmp(d.data() + record, "cmap", 4) == 0) {
				cmap = BE32(d, record + 8);
			}
		}
		if (!cmap) {
			return found;
		}

		std::size_t   table = 0;
		std::uint16_t format = 0;
		for (std::uint16_t s = 0, n = BE16(d, cmap + 2); s < n; ++s) {
			const auto platform = BE16(d, cmap + 4 + s * 8);
			const auto encoding = BE16(d, cmap + 6 + s * 8);
			const auto offset = cmap + BE32(d, cmap + 8 + s * 8);
			const auto candidate = BE16(d, offset);
			const bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));
			if (unicode && (candidate == 12 || (candidate == 4 && format != 12))) {
				table = offset;
				format = candidate;
			}
		}

		for (const auto cp : a_wanted) {
			if ((format == 12 && HasFormat12(d, table, cp)) || (format == 4 && HasFormat4(d, table, cp))) {
				found.insert(cp);
			}
		}
		return found;
	}
}
