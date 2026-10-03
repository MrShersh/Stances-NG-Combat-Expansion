#include "Forms.h"

namespace Forms
{
	namespace
	{
		constexpr auto kStancesPlugin = "StancesNG.esp"sv;
		constexpr auto kOwnPlugin = "Stances NG - Combat Expansion.esp"sv;

		// Same IDs Stances NG itself looks up (mod-data.h) and documents for OAR conditions.
		constexpr std::array<RE::FormID, 3> kStanceSpellIDs{ 0x800, 0x801, 0x802 };
		constexpr std::array<RE::FormID, 3> kStanceEffectIDs{ 0x803, 0x805, 0x806 };
		constexpr std::array<RE::FormID, 3> kIconEffectIDs{ 0x913, 0x914, 0x915 };
		constexpr RE::FormID kCurrentStanceGlobalID = 0x917;

		// Assigned by tools/esp in creation order; `dotnet run -- verify` prints them.
		constexpr RE::FormID kNoAttackSpeedBearID = 0x806;
		constexpr RE::FormID kNoAttackSpeedHawkID = 0x807;
		constexpr RE::FormID kStanceAbilityID = 0x812;
		constexpr RE::FormID kSpeedFixAbilityID = 0x813;
		constexpr RE::FormID kStancePerkID = 0x814;

		bool loaded = false;

		template <class T>
		T* Lookup(RE::FormID a_id, std::string_view a_plugin)
		{
			auto* form = RE::TESDataHandler::GetSingleton()->LookupForm<T>(a_id, a_plugin);
			if (!form) {
				logger::error("{} {:03X} is missing or has an unexpected type", a_plugin, a_id);
			}
			return form;
		}

		bool IsPluginLoaded(std::string_view a_plugin)
		{
			return RE::TESDataHandler::GetSingleton()->LookupModByName(a_plugin) != nullptr;
		}
	}

	bool Load()
	{
		loaded = false;
		if (!IsPluginLoaded(kStancesPlugin)) {
			logger::error("{} is not active, the mod stays off", kStancesPlugin);
			return false;
		}
		if (!IsPluginLoaded(kOwnPlugin)) {
			logger::error("{} is not active, the mod stays off", kOwnPlugin);
			return false;
		}

		bool ok = true;
		for (std::size_t i = 0; i < 3; ++i) {
			stanceSpells[i] = Lookup<RE::SpellItem>(kStanceSpellIDs[i], kStancesPlugin);
			stanceEffects[i] = Lookup<RE::EffectSetting>(kStanceEffectIDs[i], kStancesPlugin);
			iconEffects[i] = Lookup<RE::EffectSetting>(kIconEffectIDs[i], kStancesPlugin);
			ok = ok && stanceSpells[i] && stanceEffects[i];
		}
		currentStanceGlobal = Lookup<RE::TESGlobal>(kCurrentStanceGlobalID, kStancesPlugin);

		stanceAbility = Lookup<RE::SpellItem>(kStanceAbilityID, kOwnPlugin);
		speedFixAbility = Lookup<RE::SpellItem>(kSpeedFixAbilityID, kOwnPlugin);
		stancePerk = Lookup<RE::BGSPerk>(kStancePerkID, kOwnPlugin);
		noAttackSpeedBear = Lookup<RE::TESGlobal>(kNoAttackSpeedBearID, kOwnPlugin);
		noAttackSpeedHawk = Lookup<RE::TESGlobal>(kNoAttackSpeedHawkID, kOwnPlugin);

		loaded = ok && currentStanceGlobal && stanceAbility && speedFixAbility && stancePerk && noAttackSpeedBear &&
		         noAttackSpeedHawk;
		logger::info("Forms {}", loaded ? "loaded" : "incomplete, the mod stays off");
		return loaded;
	}

	bool IsLoaded()
	{
		return loaded;
	}
}
