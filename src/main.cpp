#include "Bonuses.h"
#include "EnemyTypes.h"
#include "Forms.h"
#include "Indicator.h"
#include "Loadout.h"
#include "Menu.h"
#include "RevertStance.h"
#include "Settings.h"
#include "Stance.h"
#include "Text.h"
#include "Visuals.h"

namespace
{
	void OnGameLoaded(const char* a_trigger)
	{
		logger::debug("Game loaded ({})", a_trigger);
		Menu::EnsurePage();
		if (!Forms::IsLoaded()) {
			return;
		}
		RevertStance::OnGameLoaded();
		Bonuses::SyncPlayer();
		// Ability magnitudes come back from the save as they were when it was made; re-adding picks up the settings.
		Bonuses::Refresh();
		StanceState::ResyncNow("game loaded");
		logger::debug("Stance after load: {}, bonuses {}", StanceState::GetName(StanceState::GetCurrent()),
			Bonuses::IsActive() ? "on" : "off");
		Bonuses::LogAppliedLater();
	}

	// In a test log on 1.7.99 the load setup never ran from SKSE's messages, so the game's own load event is a second
	// source. Running the load setup twice is harmless.
	struct LoadGameSink : RE::BSTEventSink<RE::TESLoadGameEvent>
	{
		RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*) override
		{
			SKSE::GetTaskInterface()->AddTask([] { OnGameLoaded("TESLoadGameEvent"); });
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	LoadGameSink loadGameSink;

	void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kPostPostLoad:
			// Before kDataLoaded, when Stances NG registers its FLICK page.
			Menu::InstallHook();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			Menu::Connect();
			Text::Load();
			if (Forms::Load()) {
				EnemyTypes::Tag();
				Bonuses::ApplyToForms();
				Visuals::Apply();
				RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESLoadGameEvent>(&loadGameSink);
				StanceState::Register();
				Loadout::Register();
				if (Menu::IsConnected()) {
					Indicator::Register();
				}
			}
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			OnGameLoaded("kPostLoadGame");
			break;
		case SKSE::MessagingInterface::kNewGame:
			OnGameLoaded("kNewGame");
			break;
		default:
			break;
		}
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	const auto plugin = SKSE::PluginDeclaration::GetSingleton();
	logger::info("{} {} loaded", plugin->GetName(), plugin->GetVersion().string());

	Settings::Load();
	SKSE::GetMessagingInterface()->RegisterListener(OnSKSEMessage);
	return true;
}
