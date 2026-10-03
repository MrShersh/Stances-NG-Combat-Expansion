using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;

// Builds Stances NG - Combat Expansion.esp: the records the SKSE plugin drives at runtime. Magnitudes written here are only the
// defaults; the plugin overwrites them from StancesNG.toml on load and whenever the menu changes them.
//
// Usage: dotnet run -- <output.esp>          build
//        dotnet run -- verify <file.esp>     dump what was written

if (args.Length > 1 && args[0] == "verify")
{
    Verify(args[1]);
    return;
}

var outPath = args.Length > 0 ? args[0] : "Stances NG - Combat Expansion.esp";

var skyrimEsm = ModKey.FromNameAndExtension("Skyrim.esm");
var stancesEsp = ModKey.FromNameAndExtension("StancesNG.esp");

// Stance effects of Stances NG (StancesNG.esp, read with tools/dumpesp): these are the same conditions Stances NG
// documents for Open Animation Replacer, so a stance counts as active exactly when its animations are.
var bearEffect = new FormKey(stancesEsp, 0x803); // APO_HighStanceEffect, "Aspect of the Bear"
var wolfEffect = new FormKey(stancesEsp, 0x805); // APO_MidStanceEffect, "Aspect of the Wolf"
var hawkEffect = new FormKey(stancesEsp, 0x806); // APO_LowStanceEffect, "Aspect of the Hawk"
var eitherHand = new FormKey(skyrimEsm, 0x13F44); // EquipType EitherHand, what Stances NG's abilities use

var mod = new SkyrimMod(ModKey.FromNameAndExtension("Stances NG - Combat Expansion.esp"), SkyrimRelease.SkyrimSE);
mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
mod.ModHeader.Author = "MrShersh";
mod.ModHeader.Description = "Stances NG - Combat Expansion: per-stance bonuses and penalties, settings in the Stances NG menu (FLICK).";

Keyword AddKeyword(string editorId)
{
    var k = new Keyword(mod, editorId);
    mod.Keywords.Add(k);
    return k;
}

// Only the base weapon speed fix uses its keywords (peak value modifiers with the same keyword do not stack).
// The four stance keywords are unused and stay only so every later record keeps its FormID, which the DLL
// hardcodes.
AddKeyword("SNGECE_AttackSpeedKeyword");
AddKeyword("SNGECE_AttackSpeedLeftKeyword");
AddKeyword("SNGECE_MoveSpeedKeyword");
AddKeyword("SNGECE_StaminaRateKeyword");
var kwSpeedFix = AddKeyword("SNGECE_WeaponSpeedFixKeyword");
var kwSpeedFixLeft = AddKeyword("SNGECE_WeaponSpeedFixLeftKeyword");

GlobalFloat AddGlobal(string editorId)
{
    var g = new GlobalFloat(mod, editorId) { Data = 0f };
    mod.Globals.Add(g);
    return g;
}

// 1 = the attack speed slider is at 0: the effect is switched off entirely instead of applying a zero change,
// which the original add-on offered as a workaround for mods with their own WeaponSpeedMult scaling.
var gNoAttackSpeedBear = AddGlobal("SNGECE_NoAttackSpeedBear");
var gNoAttackSpeedHawk = AddGlobal("SNGECE_NoAttackSpeedHawk");

Condition GlobalNotOne(GlobalFloat global) => new ConditionFloat
{
    CompareOperator = CompareOperator.NotEqualTo,
    ComparisonValue = 1f,
    Data = new GetGlobalValueConditionData { Global = new FormLinkOrIndex<IGlobalGetter>(new GetGlobalValueConditionData(), global.FormKey) },
};

Condition HasEffect(FormKey effect) => new ConditionFloat
{
    CompareOperator = CompareOperator.EqualTo,
    ComparisonValue = 1f,
    Data = new HasMagicEffectConditionData { MagicEffect = new FormLinkOrIndex<IMagicEffectGetter>(new HasMagicEffectConditionData(), effect) },
};

const MagicEffect.Flag hiddenFlags = MagicEffect.Flag.Recover | MagicEffect.Flag.NoHitEvent | MagicEffect.Flag.NoDuration |
                                     MagicEffect.Flag.NoArea | MagicEffect.Flag.HideInUI | MagicEffect.Flag.Painless |
                                     MagicEffect.Flag.NoHitEffect;

// Stance effects are plain value modifiers. The original add-on used peak value modifiers, but it kept each stance's
// effects in that stance's own spell; with Bear and Hawk in one ability, the Bear debuffs (same keywords, detrimental)
// did not apply in game while the Hawk buffs did.
MagicEffect AddEffect(string editorId, string name, ActorValue av, Keyword? peakKeyword, bool detrimental, GlobalFloat? offSwitch)
{
    var e = new MagicEffect(mod, editorId)
    {
        Name = name,
        Archetype = new MagicEffectArchetype
        {
            Type = peakKeyword != null ? MagicEffectArchetype.TypeEnum.PeakValueModifier : MagicEffectArchetype.TypeEnum.ValueModifier,
            ActorValue = av,
        },
        CastType = CastType.ConstantEffect,
        TargetType = TargetType.Self,
        Flags = hiddenFlags | (detrimental ? MagicEffect.Flag.Detrimental : 0),
        Keywords = peakKeyword != null ? new ExtendedList<IFormLinkGetter<IKeywordGetter>> { peakKeyword } : null,
        Description = "",
    };
    if (offSwitch != null)
        e.Conditions.Add(GlobalNotOne(offSwitch));
    mod.MagicEffects.Add(e);
    return e;
}

var bearAttack = AddEffect("SNGECE_BearAttackSpeed", "Bear Stance: Attack Speed", ActorValue.WeaponSpeedMult, null, true, gNoAttackSpeedBear);
var bearAttackLeft = AddEffect("SNGECE_BearAttackSpeedLeft", "Bear Stance: Off-Hand Attack Speed", ActorValue.LeftWeaponSpeedMultiply, null, true, gNoAttackSpeedBear);
var bearMove = AddEffect("SNGECE_BearMoveSpeed", "Bear Stance: Movement Speed", ActorValue.SpeedMult, null, true, null);
var bearStamina = AddEffect("SNGECE_BearStaminaRate", "Bear Stance: Stamina Regeneration", ActorValue.StaminaRateMult, null, true, null);
var hawkAttack = AddEffect("SNGECE_HawkAttackSpeed", "Hawk Stance: Attack Speed", ActorValue.WeaponSpeedMult, null, false, gNoAttackSpeedHawk);
var hawkAttackLeft = AddEffect("SNGECE_HawkAttackSpeedLeft", "Hawk Stance: Off-Hand Attack Speed", ActorValue.LeftWeaponSpeedMultiply, null, false, gNoAttackSpeedHawk);
var hawkMove = AddEffect("SNGECE_HawkMoveSpeed", "Hawk Stance: Movement Speed", ActorValue.SpeedMult, null, false, null);
var hawkStamina = AddEffect("SNGECE_HawkStaminaRate", "Hawk Stance: Stamina Regeneration", ActorValue.StaminaRateMult, null, false, null);
var speedFix = AddEffect("SNGECE_WeaponSpeedFix", "Base Weapon Speed", ActorValue.WeaponSpeedMult, kwSpeedFix, false, null);
var speedFixLeft = AddEffect("SNGECE_WeaponSpeedFixLeft", "Base Off-Hand Weapon Speed", ActorValue.LeftWeaponSpeedMultiply, kwSpeedFixLeft, false, null);

Spell AddAbility(string editorId, string name)
{
    var s = new Spell(mod, editorId)
    {
        Name = name,
        Type = SpellType.Ability,
        CastType = CastType.ConstantEffect,
        TargetType = TargetType.Self,
        EquipmentType = new FormLinkNullable<IEquipTypeGetter>(eitherHand),
        Description = "",
        ObjectBounds = new ObjectBounds(),
    };
    mod.Spells.Add(s);
    return s;
}

void AddSpellEffect(Spell spell, MagicEffect effect, float magnitude, FormKey? stance)
{
    var entry = new Effect
    {
        BaseEffect = new FormLinkNullable<IMagicEffectGetter>(effect.FormKey),
        Data = new EffectData { Magnitude = magnitude, Area = 0, Duration = 0 },
    };
    if (stance is { } s)
        entry.Conditions.Add(HasEffect(s));
    spell.Effects.Add(entry);
}

// Order matters: the plugin finds each effect by its base effect, but the defaults below must match Settings.h.
var stanceAbility = AddAbility("SNGECE_StanceAbility", "Stance Bonuses");
AddSpellEffect(stanceAbility, bearAttack, 0.15f, bearEffect);
AddSpellEffect(stanceAbility, bearAttackLeft, 0.15f, bearEffect);
AddSpellEffect(stanceAbility, bearMove, 10f, bearEffect);
AddSpellEffect(stanceAbility, bearStamina, 10f, bearEffect);
AddSpellEffect(stanceAbility, hawkAttack, 0.20f, hawkEffect);
AddSpellEffect(stanceAbility, hawkAttackLeft, 0.20f, hawkEffect);
AddSpellEffect(stanceAbility, hawkMove, 10f, hawkEffect);
AddSpellEffect(stanceAbility, hawkStamina, 10f, hawkEffect);

var speedFixAbility = AddAbility("SNGECE_WeaponSpeedFixAbility", "Base Weapon Speed Fix");
AddSpellEffect(speedFixAbility, speedFix, 1f, null);
AddSpellEffect(speedFixAbility, speedFixLeft, 1f, null);

// Entry order is what the plugin relies on (Perk.cpp, kEntries); priorities keep the engine from reordering them.
var perk = new Perk(mod, "SNGECE_StancePerk")
{
    Name = "Stance Bonuses",
    Description = "",
    Playable = false,
    Hidden = true,
    NumRanks = 1,
    Level = 0,
    Trait = false,
};

byte priority = 0;
void AddEntry(APerkEntryPointEffect.EntryType entryPoint, byte tabCount, float value, FormKey stance)
{
    perk.Effects.Add(new PerkEntryPointModifyValue
    {
        Rank = 0,
        Priority = priority++,
        EntryPoint = entryPoint,
        PerkConditionTabCount = tabCount,
        Modification = PerkEntryPointModifyValue.ModificationType.Multiply,
        Value = value,
        Conditions = new ExtendedList<PerkCondition>
        {
            new PerkCondition { RunOnTabIndex = 0, Conditions = new ExtendedList<Condition> { HasEffect(stance) } },
        },
    });
}

AddEntry(APerkEntryPointEffect.EntryType.ModAttackDamage, 3, 1.30f, bearEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModPowerAttackDamage, 3, 1.10f, bearEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModIncomingDamage, 3, 1.10f, bearEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModAttackDamage, 3, 0.90f, wolfEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModPercentBlocked, 1, 1.10f, wolfEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModIncomingStagger, 2, 0.90f, wolfEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModIncomingDamage, 3, 0.95f, wolfEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModAttackDamage, 3, 0.80f, hawkEffect);
AddEntry(APerkEntryPointEffect.EntryType.ModPowerAttackStamina, 2, 0.80f, hawkEffect);

// --- Attack damage bonus against enemy types (one entry per stance and type) ---
// How the game tells types apart (checked with tools/racedump against the masters): ActorType* keywords on races, plus
// a few on NPC records; HasKeyword on an actor sees both. Vampires are ActorTypeUndead (+ ActorTypeNPC). Some types
// have no reliable keyword: spriggans none at all, giants and horses only on some NPCs, frost dragons share DragonRace
// with fire dragons. For those, and as a fallback for mod races without the vanilla keyword, the plugin tags races and
// NPCs of every loaded mod at startup with the SNGECE_Enemy* keywords below (EnemyTypes.cpp: skeleton path of the race,
// frost resistance ability of a dragon). The conditions here only test keywords.
//
// An enemy can match several types (a vampire is NPC and Undead, a ghost Undead and Ghost), and the bonuses must not
// stack: each entry also requires that no type ranked above it matches. The plugin ranks the types of each stance by
// bonus (highest first, ties by matchOrder = most specific first), so exactly one, the largest, applies.
//
// !!! These keywords come after every older record so existing FormIDs stay put; EnemyTypes.cpp hardcodes 0x815+.
var kwEnemySpriggan = AddKeyword("SNGECE_EnemySpriggan");
var kwEnemyGiant = AddKeyword("SNGECE_EnemyGiant");
var kwEnemyHorse = AddKeyword("SNGECE_EnemyHorse");
var kwEnemyTroll = AddKeyword("SNGECE_EnemyTroll");
var kwEnemyDragon = AddKeyword("SNGECE_EnemyDragon");
var kwEnemyDwarven = AddKeyword("SNGECE_EnemyDwarven");
var kwEnemyFrostDragon = AddKeyword("SNGECE_EnemyFrostDragon");

// Vanilla ActorType keywords (Skyrim.esm, IDs from tools/racedump).
FormKey Vanilla(uint id) => new FormKey(skyrimEsm, id);
var actorTypeNPC = Vanilla(0x013794);
var actorTypeCreature = Vanilla(0x013795);
var actorTypeUndead = Vanilla(0x013796);
var actorTypeDaedra = Vanilla(0x013797);
var actorTypeAnimal = Vanilla(0x013798);
var actorTypeDwarven = Vanilla(0x01397A);
var actorTypeHorse = Vanilla(0x026110);
var actorTypeDragon = Vanilla(0x035D59);
var actorTypeGhost = Vanilla(0x0D205E);
var actorTypeTroll = Vanilla(0x0F5D16);
var actorTypeGiant = Vanilla(0x10E984);

Condition HasKeyword(FormKey keyword, bool want, bool or) => new ConditionFloat
{
    CompareOperator = CompareOperator.EqualTo,
    ComparisonValue = want ? 1f : 0f,
    Flags = or ? Condition.Flag.OR : 0,
    Data = new HasKeywordConditionData { Keyword = new FormLinkOrIndex<IKeywordGetter>(new HasKeywordConditionData(), keyword) },
};

// A type matches when the target has any of its keywords (one OR group); excluding it needs none of them (ANDs).
IEnumerable<Condition> Match(FormKey[] keywords) => keywords.Select((k, i) => HasKeyword(k, true, i < keywords.Length - 1));
// Exclusions are written for every other type; the plugin switches each one on (== 0) or off (>= 0, always true) so
// that only the type with the highest bonus in the stance counts. Here: on for the types earlier in matchOrder.
IEnumerable<Condition> Exclude(FormKey[] keywords, bool active) => keywords.Select(k =>
{
    var c = (ConditionFloat)HasKeyword(k, false, false);
    if (!active)
        c.CompareOperator = CompareOperator.GreaterThanOrEqualTo;
    return (Condition)c;
});

// Index = the plugin's EnemyType order (Settings.h); the entry priority encodes stance and index.
var enemyTypes = new (string Name, FormKey[] Keywords)[]
{
    ("NPC", new[] { actorTypeNPC }),
    ("Creature", new[] { actorTypeCreature }),
    ("Animal", new[] { actorTypeAnimal }),
    ("Undead", new[] { actorTypeUndead }),
    ("Daedra", new[] { actorTypeDaedra }),
    ("Dragon", new[] { actorTypeDragon, kwEnemyDragon.FormKey }),
    ("Dwarven", new[] { actorTypeDwarven, kwEnemyDwarven.FormKey }),
    ("Ghost", new[] { actorTypeGhost }),
    ("Spriggan", new[] { kwEnemySpriggan.FormKey }),
    ("Troll", new[] { actorTypeTroll, kwEnemyTroll.FormKey }),
    ("FrostDragon", new[] { kwEnemyFrostDragon.FormKey }),
    ("Horse", new[] { actorTypeHorse, kwEnemyHorse.FormKey }),
    ("Giant", new[] { actorTypeGiant, kwEnemyGiant.FormKey }),
};
// Most specific first.
var matchOrder = new[] { "FrostDragon", "Dragon", "Ghost", "Dwarven", "Giant", "Troll", "Spriggan", "Horse", "Daedra", "Undead", "Animal", "Creature", "NPC" };

var stanceEffects = new[] { bearEffect, wolfEffect, hawkEffect };
var defaultBonus = new Dictionary<(int Stance, string Type), float>
{
    [(0, "Spriggan")] = 1.10f, [(0, "Dwarven")] = 1.10f, [(0, "Giant")] = 1.10f, [(0, "Troll")] = 1.10f,
    [(1, "Undead")] = 1.25f, [(1, "Ghost")] = 1.25f,
    [(2, "NPC")] = 1.25f, [(2, "Creature")] = 1.25f,
};
for (var s = 0; s < stanceEffects.Length; s++)
{
    for (var t = 0; t < enemyTypes.Length; t++)
    {
        var type = enemyTypes[t];
        var target = new ExtendedList<Condition>(Match(type.Keywords));
        var earlierTypes = matchOrder.TakeWhile(n => n != type.Name).ToHashSet();
        foreach (var other in enemyTypes.Where(x => x.Name != type.Name))
            target.AddRange(Exclude(other.Keywords, earlierTypes.Contains(other.Name)));
        perk.Effects.Add(new PerkEntryPointModifyValue
        {
            Rank = 0,
            Priority = (byte)(100 + s * 16 + t),
            EntryPoint = APerkEntryPointEffect.EntryType.ModAttackDamage,
            PerkConditionTabCount = 3,
            Modification = PerkEntryPointModifyValue.ModificationType.Multiply,
            Value = defaultBonus.GetValueOrDefault((s, type.Name), 1.0f),
            Conditions = new ExtendedList<PerkCondition>
            {
                new PerkCondition { RunOnTabIndex = 0, Conditions = new ExtendedList<Condition> { HasEffect(stanceEffects[s]) } },
                // Tab 2 of Mod Attack Damage is the target.
                new PerkCondition { RunOnTabIndex = 2, Conditions = target },
            },
        });
    }
}mod.Perks.Add(perk);

mod.WriteToBinary(outPath);
Console.WriteLine($"Wrote {outPath}");
Verify(outPath);

static void Verify(string path)
{
    using var esp = SkyrimMod.CreateFromBinaryOverlay(path, SkyrimRelease.SkyrimSE);
    Console.WriteLine($"{Path.GetFileName(path)}: light={esp.ModHeader.Flags.HasFlag(SkyrimModHeader.HeaderFlag.Small)}");
    foreach (var m in esp.ModHeader.MasterReferences)
        Console.WriteLine($"  master {m.Master}");
    foreach (var k in esp.Keywords)
        Console.WriteLine($"  KYWD {k.FormKey} {k.EditorID}");
    foreach (var g in esp.Globals)
        Console.WriteLine($"  GLOB {g.FormKey} {g.EditorID}");
    foreach (var e in esp.MagicEffects)
        Console.WriteLine($"  MGEF {e.FormKey} {e.EditorID} {e.Archetype.Type} {e.Archetype.ActorValue} flags={e.Flags} conds={e.Conditions.Count}");
    foreach (var s in esp.Spells)
    {
        Console.WriteLine($"  SPEL {s.FormKey} {s.EditorID} {s.Type}");
        foreach (var eff in s.Effects)
            Console.WriteLine($"    {eff.BaseEffect.FormKey} mag={eff.Data?.Magnitude} conds={string.Join(",", eff.Conditions.Select(c => c.Data.GetType().Name))}");
    }
    foreach (var p in esp.Perks)
    {
        Console.WriteLine($"  PERK {p.FormKey} {p.EditorID} hidden={p.Hidden} playable={p.Playable}");
        foreach (var eff in p.Effects.OfType<IPerkEntryPointModifyValueGetter>())
            Console.WriteLine($"    prio={eff.Priority} {eff.EntryPoint} {eff.Modification} {eff.Value} tabs={eff.PerkConditionTabCount} conds={string.Join("/", eff.Conditions.Select(c => $"t{c.RunOnTabIndex}:{c.Conditions.Count}"))}");
    }
}
