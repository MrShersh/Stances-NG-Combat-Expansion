#pragma once

#include "Forms.h"

// "Revert Stance" from the original add-on: with a bow, crossbow or only spells/staves in hand the player drops to
// Neutral (the stance ability of Stances NG is taken off), and gets the stance back when a melee weapon, fists or a
// shield are equipped again.
namespace RevertStance
{
	// Re-checks the equipment; also called when the option is toggled, so turning it off gives the stance back.
	void Check();
	// A stance just showed up on the player. With the lock option on and a ranged/casting loadout it is taken off again
	// and kept for when a melee weapon comes back. True when it was taken off. Main thread.
	bool KeepNeutral(Forms::Stance a_stance);
	void OnGameLoaded();
}
