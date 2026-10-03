#include "RevertStance.h"

#include "Forms.h"
#include "Loadout.h"
#include "Settings.h"
#include "Stance.h"

namespace RevertStance
{
	namespace
	{
		using Forms::Stance;

		// Stance taken off by this module; Neutral when nothing is pending. Not saved: after a load the restore
		// falls back to Stances NG's own "current stance" global, which the game does save.
		Stance removedStance = Stance::kNeutral;
		int    globalAtRevert = 0;

		int CurrentStanceGlobal()
		{
			return Forms::currentStanceGlobal ? static_cast<int>(Forms::currentStanceGlobal->value) : 0;
		}

		Stance StanceToRestore()
		{
			// Stances NG sets this global on every hotkey press (0 = Neutral) but not for the default stance it gives
			// on a new game, so an unchanged global means "no hotkey since the revert": give back what was removed.
			const auto value = CurrentStanceGlobal();
			if (removedStance != Stance::kNeutral && value == globalAtRevert) {
				return removedStance;
			}
			// A stance in the global without its ability on the player means a revert happened before the save, or
			// the player picked that stance while reverted.
			return value >= 1 && value <= 3 ? static_cast<Stance>(value) : Stance::kNeutral;
		}

		void Revert(RE::PlayerCharacter* a_player, Stance a_stance)
		{
			removedStance = a_stance;
			globalAtRevert = CurrentStanceGlobal();
			a_player->RemoveSpell(Forms::stanceSpells[Forms::Index(a_stance)]);
			logger::debug("Revert Stance: {} taken off for a ranged/casting loadout", StanceState::GetName(a_stance));
		}

		void Restore(RE::PlayerCharacter* a_player)
		{
			const auto stance = StanceToRestore();
			removedStance = Stance::kNeutral;
			if (stance == Stance::kNeutral || StanceState::Query(a_player) != Stance::kNeutral) {
				return;
			}
			a_player->AddSpell(Forms::stanceSpells[Forms::Index(stance)]);
			logger::debug("Revert Stance: {} restored", StanceState::GetName(stance));
		}
	}

	void Check()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !Forms::IsLoaded()) {
			return;
		}
		if (Settings::revertStance.GetValue() && Loadout::IsRangedOrCaster(player)) {
			if (const auto stance = StanceState::Query(player); stance != Stance::kNeutral) {
				Revert(player, stance);
			}
		} else if (removedStance != Stance::kNeutral || Settings::revertStance.GetValue()) {
			Restore(player);
		}
	}

	bool KeepNeutral(Stance a_stance)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (a_stance == Stance::kNeutral || !player || !Settings::revertStance.GetValue() ||
			!Settings::revertStanceLock.GetValue() || !Loadout::IsRangedOrCaster(player)) {
			return false;
		}
		Revert(player, a_stance);
		return true;
	}

	void OnGameLoaded()
	{
		removedStance = Stance::kNeutral;
	}
}
