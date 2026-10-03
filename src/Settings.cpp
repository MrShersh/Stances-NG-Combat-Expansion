#include "Settings.h"

namespace Settings
{
	namespace
	{
		constexpr std::array kStanceNames{ "Bear", "Wolf", "Hawk" };

		// 39 settings built from names; REX keeps the key as a string_view, so the strings live here for good.
		struct EnemyBonusStore
		{
			EnemyBonusStore()
			{
				for (std::size_t s = 0; s < kStanceNames.size(); ++s) {
					for (std::size_t t = 0; t < kEnemyTypeCount; ++t) {
						keys[s * kEnemyTypeCount + t] = std::format("f{}Vs{}", kStanceNames[s], kEnemyTypeNames[t]);
					}
				}
				for (std::size_t s = 0; s < kStanceNames.size(); ++s) {
					for (std::size_t t = 0; t < kEnemyTypeCount; ++t) {
						values.emplace_back(kSection, keys[s * kEnemyTypeCount + t], DefaultFor(s, static_cast<EnemyType>(t)));
					}
				}
			}

			// Bear: spriggans, dwarven automatons, giants, trolls 10%. Wolf: undead, ghosts 25%. Hawk: people, creatures 25%.
			static float DefaultFor(std::size_t a_stance, EnemyType a_type)
			{
				using enum EnemyType;
				switch (a_stance) {
				case 0:
					return a_type == kSpriggan || a_type == kDwarven || a_type == kGiant || a_type == kTroll ? 10.0F : 0.0F;
				case 1:
					return a_type == kUndead || a_type == kGhost ? 25.0F : 0.0F;
				default:
					return a_type == kNPC || a_type == kCreature ? 25.0F : 0.0F;
				}
			}

			std::array<std::string, 3 * kEnemyTypeCount> keys;
			std::deque<F32>                             values;
		};

		EnemyBonusStore& Store()
		{
			static EnemyBonusStore store;
			return store;
		}
	}

	F32& EnemyBonus(std::size_t a_stance, std::size_t a_type)
	{
		return Store().values[a_stance * kEnemyTypeCount + a_type];
	}

	void Load()
	{
		Store();  // the generated settings must be registered before the store reads the file
		const auto store = REX::TOML::SettingStore::GetSingleton();
		store->Init(kFileBase, kFileUser);
		store->Load();
		ApplyLogLevel();
	}

	void Save()
	{
		REX::TOML::SettingStore::GetSingleton()->Save();
	}

	void ResetBonuses()
	{
		for (auto* setting : { &bearAttackDamage, &bearPowerAttackDamage, &bearIncomingDamage, &bearAttackSpeed, &bearMoveSpeed,
				 &bearStaminaRegen, &wolfAttackDamage, &wolfBlock, &wolfStagger, &wolfDamageResist, &hawkAttackDamage,
				 &hawkPowerAttackStamina, &hawkAttackSpeed, &hawkMoveSpeed, &hawkStaminaRegen }) {
			setting->Reset();
		}
		for (auto* setting : { &bonusesEnabled, &bonusesMeleeOnly, &fixBaseAttackSpeed, &revertStance, &revertStanceLock,
				 &hideStanceVisuals, &enemyBonusesEnabled }) {
			setting->Reset();
		}
		for (auto& setting : Store().values) {
			setting.Reset();
		}
	}

	void ApplyLogLevel()
	{
		const auto level = debugLog.GetValue() ? spdlog::level::debug : spdlog::level::info;
		if (const auto log = spdlog::default_logger(); log && log->level() != level) {
			log->set_level(level);
			log->flush_on(level);
			logger::info("Debug log {}", debugLog.GetValue() ? "on" : "off");
		}
	}
}
