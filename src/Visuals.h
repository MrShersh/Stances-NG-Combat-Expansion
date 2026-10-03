#pragma once

// Optional hiding of Stances NG's own switch visuals, leaving the indicator (and the animation set) as the only cue.
// Stances NG shows two things on a switch: the stance effect's hit shader on the body (APO_*StanceShader) and, through
// its APO_AddSpellTarget_Script, a short spell whose effect places the stance icon above the head (APO_*Icon01).
// The mod clears those two references in the loaded effect records; nothing is written to a plugin or the save,
// and turning the option off puts them back.
namespace Visuals
{
	// kDataLoaded and whenever the option changes.
	void Apply();
}
