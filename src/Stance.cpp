#include "Stance.h"

#include "Bonuses.h"
#include "RevertStance.h"

namespace StanceState
{
	namespace
	{
		std::atomic<Stance>        current{ Stance::kNeutral };
		std::atomic<std::uint32_t> changeCount{ 0 };
		std::atomic_bool           resyncQueued{ false };
		const char*                queuedReason = "";

		// Stances NG switches by removing every stance ability and adding one, so a single key press sends several
		// apply/remove events; they all collapse into one resync on the next task run.
		struct EffectSink : RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
		{
			RE::BSEventNotifyControl ProcessEvent(const RE::TESActiveEffectApplyRemoveEvent* a_event,
				RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*) override
			{
				if (a_event && a_event->target && a_event->target->IsPlayerRef()) {
					RequestResync("effect event");
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		EffectSink effectSink;
	}

	const char* GetName(Stance a_stance)
	{
		switch (a_stance) {
		case Stance::kBear:
			return "Bear";
		case Stance::kWolf:
			return "Wolf";
		case Stance::kHawk:
			return "Hawk";
		default:
			return "Neutral";
		}
	}

	Stance Query(RE::Actor* a_actor)
	{
		if (!a_actor || !Forms::IsLoaded()) {
			return Stance::kNeutral;
		}
		const auto target = a_actor->AsMagicTarget();
		if (!target) {
			return Stance::kNeutral;
		}
		for (const auto stance : Forms::kStances) {
			if (target->HasMagicEffect(Forms::stanceEffects[Forms::Index(stance)])) {
				return stance;
			}
		}
		return Stance::kNeutral;
	}

	Stance GetCurrent()
	{
		return current.load();
	}

	std::uint32_t GetChangeCount()
	{
		return changeCount.load();
	}

	void ResyncNow(const char* a_reason)
	{
		const auto stance = Query(RE::PlayerCharacter::GetSingleton());
		// Taken off again by the Revert Stance lock: its removal events bring the next resync, still Neutral.
		if (RevertStance::KeepNeutral(stance)) {
			return;
		}
		const auto previous = current.exchange(stance);
		if (previous == stance) {
			return;
		}
		changeCount.fetch_add(1);
		logger::debug("Stance: {} -> {} ({})", GetName(previous), GetName(stance), a_reason);
		Bonuses::OnStanceChanged();
	}

	void RequestResync(const char* a_reason)
	{
		if (resyncQueued.exchange(true)) {
			return;
		}
		queuedReason = a_reason;
		SKSE::GetTaskInterface()->AddTask([] {
			resyncQueued = false;
			ResyncNow(queuedReason);
		});
	}

	void Register()
	{
		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESActiveEffectApplyRemoveEvent>(&effectSink);
		logger::info("Watching the player's active effects for stance changes");
	}
}
