#pragma once

// All values are stored in Stances NG's own StancesNG.toml, section [CombatExpansion]. REX::TOML rewrites the file from its
// current content when either plugin saves, so Stances NG's keys and ours survive each other's saves.
namespace Settings
{
	inline constexpr auto kFileBase = "Data/SKSE/Plugins/StancesNG.toml";
	inline constexpr auto kFileUser = "Data/SKSE/Plugins/StancesNG_custom.toml";
	inline constexpr auto kSection = "CombatExpansion";

	// REX loads the base file as the defaults too (TSetting::Load with a_isBase), and the mod saves into that same
	// file, so REX's GetValueDefault() turns into "last saved value". The code default is kept separately for resets.
	template <class T>
	class Setting : public REX::TOML::Setting<T>
	{
	public:
		Setting(std::string_view a_section, std::string_view a_key, T a_default) :
			REX::TOML::Setting<T>(a_section, a_key, a_default),
			_codeDefault(a_default)
		{}

		[[nodiscard]] T GetCodeDefault() const { return _codeDefault; }
		void            Reset() { this->SetValue(_codeDefault); }

	private:
		T _codeDefault;
	};

	using F32 = Setting<float>;
	using Bool = Setting<bool>;

	// Upper limit of debuffs that subtract from a value the engine reads as "unset" at 0: WeaponSpeedMult 0 is normal
	// speed, so a 100% attack speed debuff did nothing in testing; the same cap keeps movement and damage above zero.
	inline constexpr float kMaxDebuff = 90.0F;

	// Percent values as shown by the sliders, same meaning and defaults as the original add-on's MCM.
	inline F32 bearAttackDamage{ kSection, "fBearAttackDamage", 30.0F };
	inline F32 bearPowerAttackDamage{ kSection, "fBearPowerAttackDamage", 10.0F };
	inline F32 bearIncomingDamage{ kSection, "fBearIncomingDamage", 10.0F };
	inline F32 bearAttackSpeed{ kSection, "fBearAttackSpeed", 15.0F };
	inline F32 bearMoveSpeed{ kSection, "fBearMoveSpeed", 10.0F };
	inline F32 bearStaminaRegen{ kSection, "fBearStaminaRegen", 10.0F };

	inline F32 wolfAttackDamage{ kSection, "fWolfAttackDamage", 10.0F };
	inline F32 wolfBlock{ kSection, "fWolfBlock", 10.0F };
	inline F32 wolfStagger{ kSection, "fWolfStagger", 10.0F };
	inline F32 wolfDamageResist{ kSection, "fWolfDamageResist", 5.0F };

	inline F32 hawkAttackDamage{ kSection, "fHawkAttackDamage", 20.0F };
	inline F32 hawkPowerAttackStamina{ kSection, "fHawkPowerAttackStamina", 20.0F };
	inline F32 hawkAttackSpeed{ kSection, "fHawkAttackSpeed", 20.0F };
	inline F32 hawkMoveSpeed{ kSection, "fHawkMoveSpeed", 10.0F };
	inline F32 hawkStaminaRegen{ kSection, "fHawkStaminaRegen", 10.0F };

	inline Bool bonusesEnabled{ kSection, "bBonusesEnabled", true };
	// Stance bonuses and penalties only while a melee weapon, a shield or fists are in hand.
	inline Bool bonusesMeleeOnly{ kSection, "bBonusesMeleeOnly", true };
	inline Bool fixBaseAttackSpeed{ kSection, "bFixBaseAttackSpeed", false };
	inline Bool revertStance{ kSection, "bRevertStance", true };
	// While reverted, a stance picked by hotkey is taken off again and kept for when a melee weapon comes back.
	inline Bool revertStanceLock{ kSection, "bRevertStanceLock", true };
	inline Bool hideStanceVisuals{ kSection, "bHideStanceVisuals", true };
	// Stance changes, applied values and the enemy type check go to the log at debug level.
	inline Bool debugLog{ kSection, "bDebugLog", false };

	inline Bool showIndicator{ kSection, "bShowIndicator", true };
	inline Bool indicatorWeaponDrawnOnly{ kSection, "bIndicatorWeaponDrawnOnly", true };
	inline Bool indicatorShowNeutral{ kSection, "bIndicatorShowNeutral", true };
	inline F32 indicatorScale{ kSection, "fIndicatorScale", 1.0F };
	// Bottom-left corner of the badge, percent of the screen; the default sits above the vanilla magicka bar.
	inline F32 indicatorPosX{ kSection, "fIndicatorPosX", 3.0F };
	inline F32 indicatorPosY{ kSection, "fIndicatorPosY", 88.0F };

	// Attack damage bonus against enemy types. Order = the perk entries' type index in Stances NG - Combat Expansion.esp (tools/esp).
	enum class EnemyType : std::uint8_t
	{
		kNPC,
		kCreature,
		kAnimal,
		kUndead,
		kDaedra,
		kDragon,
		kDwarven,
		kGhost,
		kSpriggan,
		kTroll,
		kFrostDragon,
		kHorse,
		kGiant,
		kTotal
	};
	inline constexpr std::size_t kEnemyTypeCount = static_cast<std::size_t>(EnemyType::kTotal);
	inline constexpr std::array<const char*, kEnemyTypeCount> kEnemyTypeNames{ "NPC", "Creature", "Animal", "Undead", "Daedra",
		"Dragon", "Dwarven", "Ghost", "Spriggan", "Troll", "FrostDragon", "Horse", "Giant" };

	inline Bool enemyBonusesEnabled{ kSection, "bEnemyBonusesEnabled", true };
	// Percent; stance 0..2 = Bear, Wolf, Hawk. Keys like fBearVsDwarven.
	F32& EnemyBonus(std::size_t a_stance, std::size_t a_type);

	void Load();
	void Save();
	void ResetBonuses();
	// Log level from debugLog: debug lines are written (and flushed) only when it is on.
	void ApplyLogLevel();
}
