using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

// Second research question: can frost dragons be told apart from other dragons? Lists dragon NPCs with their race and
// the spells they carry.
static class Dragons
{
    public static void Dump(IEnumerable<ISkyrimModGetter> mods)
    {
        var names = new Dictionary<FormKey, string>();
        foreach (var mod in mods)
        {
            foreach (var r in mod.Races) names[r.FormKey] = r.EditorID ?? "";
            foreach (var s in mod.Spells) names[s.FormKey] = s.EditorID ?? "";
            foreach (var s in mod.Shouts) names[s.FormKey] = s.EditorID ?? "";
        }
        Console.WriteLine("\n== Dragon NPCs (race: spells)");
        foreach (var mod in mods)
            foreach (var npc in mod.Npcs)
            {
                var race = names.GetValueOrDefault(npc.Race.FormKey, "");
                if (!race.Contains("Dragon") || race.Contains("Priest"))
                    continue;
                var spells = npc.ActorEffect?.Select(s => names.GetValueOrDefault(s.FormKey, s.FormKey.ToString())) ?? Enumerable.Empty<string>();
                Console.WriteLine($"  {npc.EditorID} [{race}]: {string.Join(", ", spells)}");
            }
    }
}
