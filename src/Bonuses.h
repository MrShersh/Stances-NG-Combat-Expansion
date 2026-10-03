#pragma once

// Writes the slider values into this mod's perk and ability (what the original add-on did with SetNthEntryValue /
// SetNthEffectMagnitude) and keeps the player's perk and abilities in line with the settings.
namespace Bonuses
{
	// Form data only: perk entry values take effect immediately, ability magnitudes on the next Refresh().
	void ApplyToForms();
	// Adds or removes the perk and abilities on the player to match the settings. Main thread.
	void SyncPlayer();
	// Re-adds the stance ability so it picks up new magnitudes and re-evaluates its stance conditions right away
	// instead of on the engine's next periodic condition check. Main thread.
	void Refresh();

	// Bonuses enabled and, with the melee-only option, a melee weapon, shield or fists in hand.
	bool IsActive();

	void OnStanceChanged();
	// Equipment changed: gives or takes the bonuses for the melee-only option. Main thread.
	void OnLoadoutChanged();

	// Writes what the player actually has to the debug log (actor values, active perk multipliers); for checking a stance
	// in game without a console. LogApplied: main thread; LogAppliedLater: once the effects have started.
	void LogApplied();
	void LogAppliedLater();
}
