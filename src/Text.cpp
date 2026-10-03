#include "Text.h"

#include "FontCheck.h"

namespace Text
{
	namespace
	{
		using Table = std::unordered_map<std::string, std::string>;

		constexpr auto kTranslationFile = "Data/Interface/Translations/StancesNGCombatExpansion_{}.txt";
		constexpr auto kFlickStyleIni = "Data/FUCKs/FUCK/defaultstyle.ini";

		Table              english;
		Table              native;
		std::string        language = "ENGLISH";
		const Table*       current = &english;
		bool               fallback = false;
		bool               fontFixable = false;
		std::string        checkedFont;
		std::set<char32_t> needed;

		std::string GameLanguage()
		{
			if (const auto ini = RE::INISettingCollection::GetSingleton()) {
				if (const auto setting = ini->GetSetting("sLanguage:General"); setting && setting->GetType() == RE::Setting::Type::kString) {
					std::string value = setting->GetString();
					std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
					return value;
				}
			}
			return "ENGLISH";
		}

		// Same format FLICK and the game use: UTF-16 LE with BOM, "key<whitespace>text" per line.
		Table ReadTable(const std::string& a_language)
		{
			Table             table;
			std::ifstream     file(std::format(kTranslationFile, a_language), std::ios::binary);
			const std::vector<char> bytes{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
			if (bytes.size() < 2 || static_cast<unsigned char>(bytes[0]) != 0xFF || static_cast<unsigned char>(bytes[1]) != 0xFE) {
				return table;
			}
			std::wstring wide((bytes.size() - 2) / 2, L'\0');
			std::memcpy(wide.data(), bytes.data() + 2, wide.size() * 2);
			const auto utf8 = SKSE::stl::utf16_to_utf8(wide).value_or(std::string{});

			std::istringstream lines(utf8);
			for (std::string line; std::getline(lines, line);) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const auto split = line.find_first_of(" \t");
				if (split == std::string::npos || line.front() != '$') {
					continue;
				}
				const auto start = line.find_first_not_of(" \t", split);
				if (start != std::string::npos) {
					table.emplace(line.substr(0, split), line.substr(start));
				}
			}
			return table;
		}

		// Characters outside ASCII the translation needs; ASCII is always in FLICK's fonts and glyph ranges.
		std::set<char32_t> NeededCharacters(const Table& a_table)
		{
			std::set<char32_t> result;
			for (const auto& [key, value] : a_table) {
				for (const auto cp : FontCheck::DecodeUtf8(value)) {
					if (cp >= 0x80) {
						result.insert(cp);
					}
				}
			}
			return result;
		}

		// FLICK builds its glyph ranges from the game's validNameChars and loads only those glyphs of its font.
		bool CanShow(std::string& a_font, std::string& a_reason)
		{
			a_font = FontCheck::FlickFontName(kFlickStyleIni);

			// Checked first: when the game's set lacks the characters, no FLICK font can show them.
			const auto scaleform = RE::BSScaleformManager::GetSingleton();
			const auto chars = scaleform ? FontCheck::DecodeUtf8(scaleform->validNameChars.c_str()) : std::vector<char32_t>{};
			const std::set<char32_t> inRanges(chars.begin(), chars.end());
			const auto missing = std::ranges::count_if(needed, [&](char32_t cp) { return !inRanges.contains(cp); });
			if (missing > 0) {
				fontFixable = false;
				a_reason = std::format("{} characters are not in the game's validNameChars (fontconfig)", missing);
				return false;
			}

			fontFixable = true;
			std::set<char32_t> inFont;
			if (a_font == "Default") {
				for (const auto cp : needed) {
					if (cp <= FontCheck::kDefaultFontLast) {
						inFont.insert(cp);
					}
				}
			} else if (const auto path = FontCheck::FindFont(a_font)) {
				inFont = FontCheck::FontCharacters(*path, needed);
			} else {
				a_reason = "font file not found";
				return false;
			}
			if (inFont.size() != needed.size()) {
				a_reason = std::format("the font lacks {} of {} characters", needed.size() - inFont.size(), needed.size());
				return false;
			}
			return true;
		}
	}

	void Load()
	{
		english = ReadTable("ENGLISH");
		language = GameLanguage();
		native = language == "ENGLISH" ? Table{} : ReadTable(language);
		needed = NeededCharacters(native);
		logger::info("Translations: {} English strings, {} for {}", english.size(), native.size(), language);
		if (const auto scaleform = RE::BSScaleformManager::GetSingleton(); scaleform && !needed.empty()) {
			const auto chars = FontCheck::DecodeUtf8(scaleform->validNameChars.c_str());
			const std::set<char32_t> inRanges(chars.begin(), chars.end());
			const auto present = std::ranges::count_if(needed, [&](char32_t cp) { return inRanges.contains(cp); });
			logger::info("Game validNameChars (FLICK's glyph set) has {} of {} characters of the {} text", present, needed.size(), language);
		}
		checkedFont.clear();
		Refresh();
	}

	void Refresh()
	{
		if (native.empty()) {
			current = &english;
			fallback = false;
			return;
		}
		std::string font, reason;
		const bool  ok = CanShow(font, reason);
		current = ok ? &native : &english;
		fallback = !ok;
		if (font != checkedFont) {
			checkedFont = font;
			if (ok) {
				logger::info("UI language: {} (FLICK font '{}')", language, font);
			} else {
				logger::info("UI language: English; FLICK font '{}' cannot show {} text: {}", font, language, reason);
			}
		}
	}

	const char* Get(const char* a_key)
	{
		if (const auto it = current->find(a_key); it != current->end()) {
			return it->second.c_str();
		}
		if (const auto it = english.find(a_key); it != english.end()) {
			return it->second.c_str();
		}
		return a_key;
	}

	bool IsFallback()
	{
		return fallback;
	}

	bool NeedsOtherFont()
	{
		return fallback && fontFixable;
	}
}
