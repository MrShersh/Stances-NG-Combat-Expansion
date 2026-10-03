#pragma once

// What the player holds, for Revert Stance and the melee-only bonuses, plus the equip watcher that re-checks both.
namespace Loadout
{
	// Ranged weapon in the right hand, or casting tools (spells, staves, scrolls) in both hands: the original add-on's
	// Revert Stance rule.
	bool IsRangedOrCaster(RE::PlayerCharacter* a_player);
	// A melee weapon in either hand, a shield, or both hands empty (fists).
	bool HasMelee(RE::PlayerCharacter* a_player);

	void Register();
}
