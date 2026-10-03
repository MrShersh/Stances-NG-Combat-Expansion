#include "EnemyTypes.h"

namespace EnemyTypes
{
	namespace
	{
		constexpr auto kOwnPlugin = "Stances NG - Combat Expansion.esp"sv;

		// Stances NG - Combat Expansion.esp, created after every older record (tools/esp).
		constexpr RE::FormID kSprigganID = 0x815;
		constexpr RE::FormID kGiantID = 0x816;
		constexpr RE::FormID kHorseID = 0x817;
		constexpr RE::FormID kTrollID = 0x818;
		constexpr RE::FormID kDragonID = 0x819;
		constexpr RE::FormID kDwarvenID = 0x81A;
		constexpr RE::FormID kFrostDragonID = 0x81B;
		constexpr RE::FormID kActorTypeDragonID = 0x035D59;  // Skyrim.esm

		struct SkeletonRule
		{
			std::string_view folder;  // lower case, as part of the skeleton path
			RE::FormID       keyword;
			const char*      name;
		};

		// Folders of the vanilla skeletons (tools/racedump): mod creatures built on these bodies reuse them.
		// "\dragon\" does not match Actors\DragonPriest\; "\dwarven" covers the spider, sphere, centurion and ballista.
		constexpr std::size_t kDragonRule = 4;  // index in kSkeletonRules
		constexpr std::array kSkeletonRules{
			SkeletonRule{ "\\spriggan\\"sv, kSprigganID, "Spriggan" },
			SkeletonRule{ "\\giant\\"sv, kGiantID, "Giant" },
			SkeletonRule{ "\\horse\\"sv, kHorseID, "Horse" },
			SkeletonRule{ "\\troll\\"sv, kTrollID, "Troll" },
			SkeletonRule{ "\\dragon\\"sv, kDragonID, "Dragon" },
			SkeletonRule{ "\\dwarven"sv, kDwarvenID, "Dwarven" },
		};

		std::string Lower(std::string_view a_text)
		{
			std::string result(a_text);
			std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			std::ranges::replace(result, '/', '\\');
			return result;
		}

		std::string_view PluginOf(const RE::TESForm* a_form)
		{
			const auto file = a_form->GetFile(0);
			return file ? file->GetFilename() : "?"sv;
		}

		bool IsVanilla(std::string_view a_plugin)
		{
			constexpr std::array kMasters{ "Skyrim.esm"sv, "Update.esm"sv, "Dawnguard.esm"sv, "HearthFires.esm"sv, "Dragonborn.esm"sv };
			return std::ranges::any_of(kMasters, [&](std::string_view m) { return _stricmp(m.data(), std::string(a_plugin).c_str()) == 0; });
		}

		bool HasFrostResistance(const RE::TESNPC* a_npc)
		{
			const auto list = a_npc->actorEffects;
			if (!list || !list->spells) {
				return false;
			}
			for (std::uint32_t i = 0; i < list->numSpells; ++i) {
				const auto spell = list->spells[i];
				if (!spell || spell->GetSpellType() != RE::MagicSystem::SpellType::kAbility) {
					continue;
				}
				for (const auto* effect : spell->effects) {
					const auto* base = effect ? effect->baseEffect : nullptr;
					if (base && base->GetArchetype() == RE::EffectSetting::Archetype::kValueModifier &&
						base->data.primaryAV == RE::ActorValue::kResistFrost && !base->IsDetrimental() && effect->effectItem.magnitude > 0.0F) {
						return true;
					}
				}
			}
			return false;
		}
	}

	void Tag()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		std::array<RE::BGSKeyword*, kSkeletonRules.size()> keywords{};
		for (std::size_t i = 0; i < kSkeletonRules.size(); ++i) {
			keywords[i] = dataHandler->LookupForm<RE::BGSKeyword>(kSkeletonRules[i].keyword, kOwnPlugin);
		}
		const auto frostKeyword = dataHandler->LookupForm<RE::BGSKeyword>(kFrostDragonID, kOwnPlugin);
		const auto dragonKeyword = dataHandler->LookupForm<RE::BGSKeyword>(kActorTypeDragonID, "Skyrim.esm"sv);
		if (std::ranges::any_of(keywords, [](auto* k) { return k == nullptr; }) || !frostKeyword || !dragonKeyword) {
			logger::error("Enemy type keywords missing from Stances NG - Combat Expansion.esp; races of other mods are not tagged");
			return;
		}

		std::array<std::size_t, kSkeletonRules.size()> counts{};
		for (auto* race : dataHandler->GetFormArray<RE::TESRace>()) {
			if (!race) {
				continue;
			}
			const auto skeleton = Lower(race->skeletonModels[RE::SEXES::kMale].GetModel());
			for (std::size_t i = 0; i < kSkeletonRules.size(); ++i) {
				if (skeleton.contains(kSkeletonRules[i].folder) && !race->HasKeyword(keywords[i])) {
					race->AddKeyword(keywords[i]);
					++counts[i];
					if (!IsVanilla(PluginOf(race))) {
						logger::debug("Enemy type {}: race {} from {}", kSkeletonRules[i].name, race->GetFormEditorID(), PluginOf(race));
					}
				}
			}
		}

		std::size_t frostDragons = 0;
		for (auto* npc : dataHandler->GetFormArray<RE::TESNPC>()) {
			const auto race = npc ? npc->GetRace() : nullptr;
			if (!race || !(race->HasKeyword(dragonKeyword) || race->HasKeyword(keywords[kDragonRule])) || !HasFrostResistance(npc)) {
				continue;
			}
			if (!npc->HasKeyword(frostKeyword)) {
				npc->AddKeyword(frostKeyword);
				++frostDragons;
			}
		}

		std::string summary;
		for (std::size_t i = 0; i < kSkeletonRules.size(); ++i) {
			summary += std::format("{} {}, ", kSkeletonRules[i].name, counts[i]);
		}
		logger::info("Enemy types tagged: {}frost dragon NPCs {}", summary, frostDragons);
	}
}
