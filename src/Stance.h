#pragma once

#include "Forms.h"

// Tracks which Stances NG stance the player is in. Stances NG has no API or event of its own; a stance is its ability
// on the player, so the source of truth is the stance effect, the same thing its OAR conditions check.
namespace StanceState
{
	using Forms::Stance;

	[[nodiscard]] Stance Query(RE::Actor* a_actor);
	// Cached result of the last resync; cheap enough to call every frame.
	[[nodiscard]] Stance GetCurrent();
	// Grows on every change, so the indicator can animate a switch without its own bookkeeping.
	[[nodiscard]] std::uint32_t GetChangeCount();

	// Re-reads the player's stance on the main thread, at most once per frame however often it is requested.
	void RequestResync(const char* a_reason);
	void ResyncNow(const char* a_reason);

	void Register();
	[[nodiscard]] const char* GetName(Stance a_stance);
}
