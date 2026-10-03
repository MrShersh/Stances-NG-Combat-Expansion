#pragma once

namespace Forms
{
	enum class Stance : std::uint8_t
	{
		kNeutral,
		kBear,
		kWolf,
		kHawk,
	};

	inline constexpr std::array kStances{ Stance::kBear, Stance::kWolf, Stance::kHawk };

	// Stances NG (StancesNG.esp): one ability per stance, each carrying one stance effect.
	inline RE::SpellItem*   stanceSpells[3]{};
	inline RE::EffectSetting* stanceEffects[3]{};
	// Effects of the short spells Stances NG casts on a switch to show the stance icon above the head. Optional.
	inline RE::EffectSetting* iconEffects[3]{};
	// APO_CurrentStance: 1..3 = last stance chosen with a Stances NG hotkey; kept in the save by the game.
	inline RE::TESGlobal* currentStanceGlobal = nullptr;

	// This mod (Stances NG - Combat Expansion.esp).
	inline RE::SpellItem* stanceAbility = nullptr;
	inline RE::SpellItem* speedFixAbility = nullptr;
	inline RE::BGSPerk*   stancePerk = nullptr;
	inline RE::TESGlobal* noAttackSpeedBear = nullptr;
	inline RE::TESGlobal* noAttackSpeedHawk = nullptr;

	[[nodiscard]] constexpr std::size_t Index(Stance a_stance) { return static_cast<std::size_t>(a_stance) - 1; }

	// False when either plugin is missing or its records are not what this build expects; the mod then stays off.
	bool Load();
	[[nodiscard]] bool IsLoaded();
}
