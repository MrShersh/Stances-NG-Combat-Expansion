#include "Loadout.h"

#include "Bonuses.h"
#include "RevertStance.h"

namespace Loadout
{
	namespace
	{
		enum class Hand
		{
			kEmpty,
			kMelee,
			kRanged,
			kCaster,  // spell, scroll or staff
			kOther,   // shield, torch
		};

		Hand Classify(RE::TESForm* a_object)
		{
			if (!a_object) {
				return Hand::kEmpty;
			}
			if (const auto weapon = a_object->As<RE::TESObjectWEAP>()) {
				if (weapon->IsBow() || weapon->IsCrossbow()) {
					return Hand::kRanged;
				}
				return weapon->IsStaff() ? Hand::kCaster : Hand::kMelee;
			}
			return a_object->Is(RE::FormType::Spell, RE::FormType::Scroll) ? Hand::kCaster : Hand::kOther;
		}

		bool IsShield(RE::TESForm* a_object)
		{
			const auto armor = a_object ? a_object->As<RE::TESObjectARMO>() : nullptr;
			return armor && armor->IsShield();
		}

		std::atomic_bool checkQueued{ false };

		struct EquipSink : RE::BSTEventSink<RE::TESEquipEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) override
			{
				if (a_event && a_event->actor && a_event->actor->IsPlayerRef() && !checkQueued.exchange(true)) {
					// The event fires mid-equip; the hands are read once the equip has gone through.
					SKSE::GetTaskInterface()->AddTask([] {
						checkQueued = false;
						RevertStance::Check();
						Bonuses::OnLoadoutChanged();
					});
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		EquipSink equipSink;
	}

	bool IsRangedOrCaster(RE::PlayerCharacter* a_player)
	{
		const auto right = Classify(a_player->GetEquippedObject(false));
		const auto left = Classify(a_player->GetEquippedObject(true));
		return right == Hand::kRanged || (right == Hand::kCaster && left == Hand::kCaster);
	}

	bool HasMelee(RE::PlayerCharacter* a_player)
	{
		const auto rightObject = a_player->GetEquippedObject(false);
		const auto leftObject = a_player->GetEquippedObject(true);
		const auto right = Classify(rightObject);
		const auto left = Classify(leftObject);
		return right == Hand::kMelee || left == Hand::kMelee || IsShield(leftObject) ||
		       (right == Hand::kEmpty && left == Hand::kEmpty);
	}

	void Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(&equipSink);
	}
}
