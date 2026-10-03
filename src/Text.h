#pragma once

// UI strings of this mod. FLICK's translation lookup is not used: FLICK picks the file by game language even when
// its current font cannot draw it, and its default font (embedded Futura Std, Latin-1 only) has no Cyrillic, so a
// Russian game got unreadable text. Here the game-language strings are used only if the FLICK font has every glyph
// they need and FLICK's glyph ranges (built from the game's validNameChars) include them; otherwise English.
namespace Text
{
	// kDataLoaded: reads Interface/Translations/StancesNGCombatExpansion_ENGLISH.txt and the game language's file.
	void Load();
	// Re-checks the FLICK font (it can be changed in FLICK's settings at any time). Cheap; called when the page opens.
	void Refresh();
	// Text for a "$SNGECE_..." key in the chosen language, English if missing there, the key itself as a last resort.
	[[nodiscard]] const char* Get(const char* a_key);
	// The game language has a translation that the current FLICK font cannot show, so English is used instead.
	[[nodiscard]] bool IsFallback();
	// The fallback would go away with another FLICK font (as opposed to the game not giving FLICK the characters).
	[[nodiscard]] bool NeedsOtherFont();
}
