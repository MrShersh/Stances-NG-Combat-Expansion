#include "Bonuses.h"

#include "Forms.h"
#include "Loadout.h"
#include "Settings.h"
#include "Stance.h"

#include <thread>

namespace Bonuses
{
	namespace
	{
		using EntryPoint = RE::BGSEntryPoint::ENTRY_POINT;
		using Stance = Forms::Stance;
		using FunctionID = RE::FUNCTION_DATA::FunctionID;

		constexpr auto kOwnPlugin = "Stances NG - Combat Expansion.esp"sv;

		float Up(const Settings::F32& a_percent) { return 1.0F + a_percent.GetValue() / 100.0F; }
		float Down(const Settings::F32& a_percent) { return 1.0F - a_percent.GetValue() / 100.0F; }
		// Values saved before the cap (or edited by hand) are cut to it as well.
		float Capped(const Settings::F32& a_percent) { return std::min(a_percent.GetValue(), Settings::kMaxDebuff); }
		float CappedDown(const Settings::F32& a_percent) { return 1.0F - Capped(a_percent) / 100.0F; }

		// Perk entry point multipliers, matched by entry point and the stance in their HasMagicEffect condition.
		struct PerkValue
		{
			EntryPoint entryPoint;
			Stance     stance;
			float (*value)();
		};

		const std::array kPerkValues{
			PerkValue{ EntryPoint::kModAttackDamage, Stance::kBear, [] { return Up(Settings::bearAttackDamage); } },
			PerkValue{ EntryPoint::kModPowerAttackDamage, Stance::kBear, [] { return Up(Settings::bearPowerAttackDamage); } },
			PerkValue{ EntryPoint::kModIncomingDamage, Stance::kBear, [] { return Up(Settings::bearIncomingDamage); } },
			PerkValue{ EntryPoint::kModAttackDamage, Stance::kWolf, [] { return CappedDown(Settings::wolfAttackDamage); } },
			PerkValue{ EntryPoint::kModPercentBlocked, Stance::kWolf, [] { return Up(Settings::wolfBlock); } },
			PerkValue{ EntryPoint::kModIncomingStagger, Stance::kWolf, [] { return Down(Settings::wolfStagger); } },
			PerkValue{ EntryPoint::kModIncomingDamage, Stance::kWolf, [] { return Down(Settings::wolfDamageResist); } },
			PerkValue{ EntryPoint::kModAttackDamage, Stance::kHawk, [] { return CappedDown(Settings::hawkAttackDamage); } },
			PerkValue{ EntryPoint::kModPowerAttackStamina, Stance::kHawk, [] { return Down(Settings::hawkPowerAttackStamina); } },
		};

		// Stance ability magnitudes by base effect (Stances NG - Combat Expansion.esp IDs, see tools/esp). Attack speed is a
		// WeaponSpeedMult fraction, movement SpeedMult points, stamina StaminaRateMult percent, as in the original.
		struct AbilityValue
		{
			RE::FormID effectID;
			float (*value)();
		};

		const std::array kAbilityValues{
			AbilityValue{ 0x808, [] { return Capped(Settings::bearAttackSpeed) / 100.0F; } },
			AbilityValue{ 0x809, [] { return Capped(Settings::bearAttackSpeed) / 100.0F; } },
			AbilityValue{ 0x80A, [] { return Capped(Settings::bearMoveSpeed); } },
			AbilityValue{ 0x80B, [] { return Settings::bearStaminaRegen.GetValue(); } },
			AbilityValue{ 0x80C, [] { return Settings::hawkAttackSpeed.GetValue() / 100.0F; } },
			AbilityValue{ 0x80D, [] { return Settings::hawkAttackSpeed.GetValue() / 100.0F; } },
			AbilityValue{ 0x80E, [] { return Settings::hawkMoveSpeed.GetValue(); } },
			AbilityValue{ 0x80F, [] { return Settings::hawkStaminaRegen.GetValue(); } },
		};

		// Enemy type entries carry priority 100 + stance * 16 + type (tools/esp); the stat entries use 0..8.
		constexpr std::uint8_t kEnemyPriorityBase = 100;

		std::optional<std::pair<std::size_t, std::size_t>> EnemySlot(const RE::BGSEntryPointPerkEntry* a_entry)
		{
			const auto priority = a_entry->header.priority;
			if (priority < kEnemyPriorityBase) {
				return std::nullopt;
			}
			const std::size_t stance = (priority - kEnemyPriorityBase) / 16;
			const std::size_t type = (priority - kEnemyPriorityBase) % 16;
			if (stance >= 3 || type >= Settings::kEnemyTypeCount) {
				return std::nullopt;
			}
			return std::pair{ stance, type };
		}

		float EnemyMultiplier(std::size_t a_stance, std::size_t a_type)
		{
			if (!Settings::enemyBonusesEnabled.GetValue()) {
				return 1.0F;
			}
			return 1.0F + std::max(Settings::EnemyBonus(a_stance, a_type).GetValue(), 0.0F) / 100.0F;
		}

		// Tie-break when two matching types have the same bonus: the more specific one (tools/esp matchOrder).
		constexpr std::array kSpecificity{ Settings::EnemyType::kFrostDragon, Settings::EnemyType::kDragon, Settings::EnemyType::kGhost,
			Settings::EnemyType::kDwarven, Settings::EnemyType::kGiant, Settings::EnemyType::kTroll, Settings::EnemyType::kSpriggan,
			Settings::EnemyType::kHorse, Settings::EnemyType::kDaedra, Settings::EnemyType::kUndead, Settings::EnemyType::kAnimal,
			Settings::EnemyType::kCreature, Settings::EnemyType::kNPC };

		// Lower ranks first: the highest bonus, then the most specific type.
		std::pair<float, std::size_t> EnemyRank(std::size_t a_stance, std::size_t a_type)
		{
			const auto specificity = static_cast<std::size_t>(
				std::ranges::find(kSpecificity, static_cast<Settings::EnemyType>(a_type)) - kSpecificity.begin());
			return { -EnemyMultiplier(a_stance, a_type), specificity };
		}

		// Each enemy entry excludes every other type with "HasKeyword == 0" conditions (tools/esp). Only the exclusions
		// of types ranked above the entry's own type stay active; the rest become ">= 0", which is always true. So an enemy
		// of several types gets exactly one bonus, the largest. Conditions are evaluated live, so this takes effect at once.
		void RankEnemyEntries(const std::vector<std::tuple<RE::BGSEntryPointPerkEntry*, std::size_t, std::size_t>>& a_entries)
		{
			using OpCode = RE::CONDITION_ITEM_DATA::OpCode;
			constexpr std::size_t kTargetTab = 2;

			// A type's own keywords are the entry's "== 1" conditions.
			std::unordered_map<const void*, std::size_t> typeOfKeyword;
			for (const auto& [entry, stance, type] : a_entries) {
				if (entry->conditions.size() <= kTargetTab) {
					continue;
				}
				for (auto item = entry->conditions[kTargetTab].head; item; item = item->next) {
					if (item->data.functionData.function == FunctionID::kHasKeyword && item->data.comparisonValue.f == 1.0F) {
						typeOfKeyword[item->data.functionData.params[0]] = type;
					}
				}
			}

			std::size_t unknown = 0;
			for (const auto& [entry, stance, type] : a_entries) {
				if (entry->conditions.size() <= kTargetTab) {
					continue;
				}
				const auto ownRank = EnemyRank(stance, type);
				for (auto item = entry->conditions[kTargetTab].head; item; item = item->next) {
					if (item->data.functionData.function != FunctionID::kHasKeyword || item->data.comparisonValue.f != 0.0F) {
						continue;
					}
					const auto other = typeOfKeyword.find(item->data.functionData.params[0]);
					if (other == typeOfKeyword.end()) {
						++unknown;
						continue;
					}
					const bool active = EnemyRank(stance, other->second) < ownRank;
					item->data.flags.opCode = active ? OpCode::kEqualTo : OpCode::kGreaterThanOrEqualTo;
				}
			}
			if (unknown > 0) {
				logger::error("Enemy type entries: {} exclusion conditions with an unknown keyword", unknown);
			}
		}

		// Evaluates the entries' target conditions for a few typical enemies, the way the engine groups them (consecutive
		// OR items form a group, groups are ANDed), and logs which bonus each would get. Proves in the log that exactly
		// one, the largest, applies; only logs when the outcome changes.
		void CheckEnemyEntries(const std::vector<std::tuple<RE::BGSEntryPointPerkEntry*, std::size_t, std::size_t>>& a_entries)
		{
			const auto dh = RE::TESDataHandler::GetSingleton();
			auto vanilla = [&](RE::FormID a_id) { return dh->LookupForm<RE::BGSKeyword>(a_id, "Skyrim.esm"sv); };
			auto own = [&](RE::FormID a_id) { return dh->LookupForm<RE::BGSKeyword>(a_id, kOwnPlugin); };
			const auto npc = vanilla(0x013794), creature = vanilla(0x013795), undead = vanilla(0x013796), daedra = vanilla(0x013797),
					   animal = vanilla(0x013798), dragon = vanilla(0x035D59), ghost = vanilla(0x0D205E), troll = vanilla(0x0F5D16);
			const auto spriggan = own(0x815), trollTag = own(0x818), dragonTag = own(0x819), frost = own(0x81B);

			const std::array<std::pair<const char*, std::vector<RE::BGSKeyword*>>, 8> profiles{ {
				{ "bandit", { npc } },
				{ "vampire", { npc, undead } },
				{ "ghost", { npc, undead, ghost } },
				{ "draugr", { creature, undead } },
				{ "dremora", { npc, daedra } },
				{ "troll", { animal, creature, troll, trollTag } },
				{ "spriggan", { creature, spriggan } },
				{ "frost dragon", { creature, dragon, dragonTag, frost } },
			} };

			auto holds = [](const RE::TESConditionItem* a_item, const std::vector<RE::BGSKeyword*>& a_keywords) {
				const float has = std::ranges::find(a_keywords, a_item->data.functionData.params[0]) != a_keywords.end() ? 1.0F : 0.0F;
				const float want = a_item->data.comparisonValue.f;
				using OpCode = RE::CONDITION_ITEM_DATA::OpCode;
				switch (a_item->data.flags.opCode) {
				case OpCode::kEqualTo:
					return has == want;
				case OpCode::kNotEqualTo:
					return has != want;
				case OpCode::kGreaterThanOrEqualTo:
					return has >= want;
				default:
					return false;
				}
			};
			auto fires = [&](RE::BGSEntryPointPerkEntry* a_entry, const std::vector<RE::BGSKeyword*>& a_keywords) {
				bool all = true, group = false;
				for (auto item = a_entry->conditions[2].head; item; item = item->next) {
					group = group || holds(item, a_keywords);
					if (!item->data.flags.isOR) {
						all = all && group;
						group = false;
					}
				}
				return all;
			};

			static std::string last;
			std::string report;
			int         stacked = 0;
			for (std::size_t stance = 0; stance < 3; ++stance) {
				report += std::format("\n  {}:", StanceState::GetName(static_cast<Stance>(stance + 1)));
				for (const auto& [name, keywords] : profiles) {
					std::string winners;
					int         count = 0;
					for (const auto& [entry, entryStance, type] : a_entries) {
						if (entryStance == stance && entry->conditions.size() > 2 && fires(entry, keywords)) {
							winners += std::format("{}{} x{:.2f}", winners.empty() ? "" : " + ", Settings::kEnemyTypeNames[type], EnemyMultiplier(stance, type));
							++count;
						}
					}
					report += std::format(" {} -> {};", name, winners.empty() ? "none" : winners);
					stacked += count > 1 ? 1 : 0;
				}
			}
			if (stacked > 0) {
				logger::warn("Enemy type bonuses stack for {} stance/enemy cases; the ESP does not fit this DLL", stacked);
			}
			if (Settings::debugLog.GetValue() && report != last) {
				last = report;
				logger::debug("Enemy type bonus check (what a hit would get):{}", report);
			}
		}

		Stance StanceOf(const RE::EffectSetting* a_effect)
		{
			for (const auto stance : Forms::kStances) {
				if (a_effect && a_effect == Forms::stanceEffects[Forms::Index(stance)]) {
					return stance;
				}
			}
			return Stance::kNeutral;
		}

		Stance ConditionStance(RE::BGSEntryPointPerkEntry* a_entry)
		{
			if (a_entry->conditions.empty()) {
				return Stance::kNeutral;
			}
			for (auto item = a_entry->conditions[0].head; item; item = item->next) {
				const auto& function = item->data.functionData;
				if (function.function == FunctionID::kHasMagicEffect) {
					return StanceOf(static_cast<RE::EffectSetting*>(function.params[0]));
				}
			}
			return Stance::kNeutral;
		}

		void ApplyPerk()
		{
			std::size_t matched = 0;
			std::size_t enemyMatched = 0;
			std::vector<std::tuple<RE::BGSEntryPointPerkEntry*, std::size_t, std::size_t>> enemyEntries;
			for (auto* entry : Forms::stancePerk->perkEntries) {
				if (!entry || entry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint) {
					continue;
				}
				auto*      entryPoint = static_cast<RE::BGSEntryPointPerkEntry*>(entry);
				auto*      data = entryPoint->functionData;
				const auto stance = ConditionStance(entryPoint);
				if (!data || data->GetType() != RE::BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA::kOneValue) {
					continue;
				}
				if (const auto slot = EnemySlot(entryPoint)) {
					static_cast<RE::BGSEntryPointFunctionDataOneValue*>(data)->data = EnemyMultiplier(slot->first, slot->second);
					++enemyMatched;
					enemyEntries.emplace_back(entryPoint, slot->first, slot->second);
					continue;
				}
				for (const auto& value : kPerkValues) {
					if (value.entryPoint == entryPoint->entryData.entryPoint.get() && value.stance == stance) {
						static_cast<RE::BGSEntryPointFunctionDataOneValue*>(data)->data = value.value();
						++matched;
						break;
					}
				}
			}
			if (matched != kPerkValues.size()) {
				logger::error("Stance perk: {} of {} entries matched, Stances NG - Combat Expansion.esp does not fit this DLL", matched, kPerkValues.size());
			}
			RankEnemyEntries(enemyEntries);
			CheckEnemyEntries(enemyEntries);
			if (enemyMatched != 3 * Settings::kEnemyTypeCount) {
				logger::error("Stance perk: {} of {} enemy type entries found, Stances NG - Combat Expansion.esp does not fit this DLL", enemyMatched, 3 * Settings::kEnemyTypeCount);
			}
		}

		void ApplyAbility()
		{
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			std::size_t matched = 0;
			for (auto* effect : Forms::stanceAbility->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}
				for (const auto& value : kAbilityValues) {
					if (effect->baseEffect == dataHandler->LookupForm<RE::EffectSetting>(value.effectID, kOwnPlugin)) {
						effect->effectItem.magnitude = value.value();
						++matched;
						break;
					}
				}
			}
			if (matched != kAbilityValues.size()) {
				logger::error("Stance ability: {} of {} effects matched, Stances NG - Combat Expansion.esp does not fit this DLL", matched, kAbilityValues.size());
			}

			Forms::noAttackSpeedBear->value = Settings::bearAttackSpeed.GetValue() == 0.0F ? 1.0F : 0.0F;
			Forms::noAttackSpeedHawk->value = Settings::hawkAttackSpeed.GetValue() == 0.0F ? 1.0F : 0.0F;
		}

		const char* EntryPointName(EntryPoint a_entryPoint)
		{
			switch (a_entryPoint) {
			case EntryPoint::kModAttackDamage:
				return "AttackDamage";
			case EntryPoint::kModPowerAttackDamage:
				return "PowerAttackDamage";
			case EntryPoint::kModIncomingDamage:
				return "IncomingDamage";
			case EntryPoint::kModPercentBlocked:
				return "Block";
			case EntryPoint::kModIncomingStagger:
				return "IncomingStagger";
			case EntryPoint::kModPowerAttackStamina:
				return "PowerAttackStamina";
			default:
				return "Other";
			}
		}

		void SetPerk(RE::PlayerCharacter* a_player, RE::BGSPerk* a_perk, bool a_has)
		{
			if (a_player->HasPerk(a_perk) != a_has) {
				a_has ? a_player->AddPerk(a_perk) : a_player->RemovePerk(a_perk);
			}
		}

		void SetSpell(RE::PlayerCharacter* a_player, RE::SpellItem* a_spell, bool a_has)
		{
			if (a_player->HasSpell(a_spell) != a_has) {
				a_has ? a_player->AddSpell(a_spell) : a_player->RemoveSpell(a_spell);
			}
		}

		// The engine recomputes movement speed when carry weight changes, not when SpeedMult does.
		void NudgeSpeed(RE::PlayerCharacter* a_player)
		{
			const auto av = a_player->AsActorValueOwner();
			av->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kCarryWeight, 0.1F);
			av->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kCarryWeight, -0.1F);
		}
	}

	void ApplyToForms()
	{
		if (!Forms::IsLoaded()) {
			return;
		}
		ApplyPerk();
		ApplyAbility();
	}

	bool IsActive()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		return player && Settings::bonusesEnabled.GetValue() &&
		       (!Settings::bonusesMeleeOnly.GetValue() || Loadout::HasMelee(player));
	}

	void SyncPlayer()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !Forms::IsLoaded()) {
			return;
		}
		const bool active = IsActive();
		const bool hadAbility = player->HasSpell(Forms::stanceAbility);
		SetPerk(player, Forms::stancePerk, active);
		SetSpell(player, Forms::stanceAbility, active);
		SetSpell(player, Forms::speedFixAbility, Settings::fixBaseAttackSpeed.GetValue());
		if (hadAbility != active) {
			NudgeSpeed(player);
		}
	}

	void Refresh()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !Forms::IsLoaded() || !IsActive()) {
			return;
		}
		if (player->HasSpell(Forms::stanceAbility)) {
			player->RemoveSpell(Forms::stanceAbility);
		}
		player->AddSpell(Forms::stanceAbility);
		NudgeSpeed(player);
	}

	void OnStanceChanged()
	{
		// Also covers a game load that slipped past the load handlers: the perk would otherwise never be given.
		SyncPlayer();
		Refresh();
		LogAppliedLater();
	}

	void OnLoadoutChanged()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !Forms::IsLoaded()) {
			return;
		}
		const bool had = player->HasPerk(Forms::stancePerk);
		SyncPlayer();
		if (had != player->HasPerk(Forms::stancePerk)) {
			logger::debug("Stance bonuses {} for the equipped weapons", had ? "off" : "on");
			LogAppliedLater();
		}
	}

	void LogAppliedLater()
	{
		if (!Settings::debugLog.GetValue()) {
			return;
		}
		// Ability effects start over the next frames; reading them right away would show the previous stance.
		std::thread([] {
			std::this_thread::sleep_for(400ms);
			SKSE::GetTaskInterface()->AddTask([] { LogApplied(); });
		}).detach();
	}

	void LogApplied()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !Forms::IsLoaded()) {
			return;
		}
		const auto av = player->AsActorValueOwner();
		std::string multipliers;
		std::string enemies;
		for (auto* entry : Forms::stancePerk->perkEntries) {
			if (!entry || entry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint) {
				continue;
			}
			auto* entryPoint = static_cast<RE::BGSEntryPointPerkEntry*>(entry);
			if (entryPoint->conditions.empty() || !entryPoint->conditions[0].IsTrue(player, player)) {
				continue;
			}
			const auto* data = entryPoint->functionData;
			if (!data || data->GetType() != RE::BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA::kOneValue) {
				continue;
			}
			const float value = static_cast<const RE::BGSEntryPointFunctionDataOneValue*>(data)->data;
			if (const auto slot = EnemySlot(entryPoint)) {
				if (value != 1.0F) {
					enemies += std::format(" {} x{:.2f};", Settings::kEnemyTypeNames[slot->second], value);
				}
				continue;
			}
			multipliers += std::format(" {} x{:.2f};", EntryPointName(entryPoint->entryData.entryPoint.get()), value);
		}
		logger::debug("Applied ({}): perk {}, ability {}; WeaponSpeedMult {:.2f}, LeftWeaponSpeedMultiply {:.2f}, SpeedMult {:.1f}, StaminaRateMult {:.1f};{}",
			StanceState::GetName(StanceState::GetCurrent()), player->HasPerk(Forms::stancePerk) ? "yes" : "NO",
			player->HasSpell(Forms::stanceAbility) ? "yes" : "NO", av->GetActorValue(RE::ActorValue::kWeaponSpeedMult),
			av->GetActorValue(RE::ActorValue::kLeftWeaponSpeedMultiply), av->GetActorValue(RE::ActorValue::kSpeedMult),
			av->GetActorValue(RE::ActorValue::kStaminaRateMult), multipliers.empty() ? " no perk multipliers" : multipliers);
		if (!enemies.empty()) {
			logger::debug("  vs enemy types:{}", enemies);
		}
	}
}
