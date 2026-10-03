#pragma once

// Tags races and NPCs of every loaded plugin with this mod's SNGECE_Enemy* keywords, so the enemy-type damage bonuses
// in Stances NG - Combat Expansion.esp also work on enemies from other mods. Only types the game does not mark reliably by itself:
//   race skeleton under Actors\Spriggan\, \Giant\, \Horse\, \Troll\, \Dragon\, \Dwarven*  -> that type
//   an NPC of a dragon race with a frost resistance ability (vanilla: AbDragonFrost)       -> frost dragon
// The keywords are added in memory at startup; no plugin or save is changed.
namespace EnemyTypes
{
	// kDataLoaded, after Forms::Load.
	void Tag();
}
