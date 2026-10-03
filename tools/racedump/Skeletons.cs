using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

// Third research question: which signals survive in races and NPCs that other mods add? Prints each race's skeleton
// path, and the effects of the dragon element abilities.
static class Skeletons
{
    public static void Dump(IReadOnlyList<ISkyrimModGetter> mods)
    {
        var races = new Dictionary<FormKey, IRaceGetter>();
        foreach (var mod in mods)
            foreach (var race in mod.Races)
                races[race.FormKey] = race;
        Console.WriteLine("\n== Race skeletons");
        foreach (var race in races.Values.OrderBy(r => r.EditorID))
            Console.WriteLine($"  {race.EditorID}: {race.SkeletalModel?.Male?.File.GivenPath}");

        var effects = new Dictionary<FormKey, IMagicEffectGetter>();
        foreach (var mod in mods)
            foreach (var e in mod.MagicEffects)
                effects[e.FormKey] = e;
        Console.WriteLine("\n== Dragon element abilities");
        foreach (var mod in mods)
            foreach (var spell in mod.Spells.Where(s => s.EditorID is "AbDragonFrost" or "AbDragonFire"))
                foreach (var eff in spell.Effects)
                {
                    var mgef = effects.GetValueOrDefault(eff.BaseEffect.FormKey);
                    Console.WriteLine($"  {spell.EditorID}: {mgef?.EditorID} {mgef?.Archetype.Type} {mgef?.Archetype.ActorValue} mag={eff.Data?.Magnitude} detrimental={mgef?.Flags.HasFlag(MagicEffect.Flag.Detrimental)}");
                }
    }
}
