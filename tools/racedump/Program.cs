using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

// Research tool: how the game tells enemy types apart. Prints the ActorType* (and related) keywords of every race in
// the given masters, and which keywords NPC records carry directly.
// Usage: dotnet run -- <Data folder> [master...]

var data = args[0];
var masterArgs = args.Skip(1).Where(a => !a.StartsWith("--")).ToArray();
var masters = masterArgs.Length > 0 ? masterArgs : new[] { "Skyrim.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Dragonborn.esm" };

var keywordNames = new Dictionary<FormKey, string>();
var mods = masters.Select(m => SkyrimMod.CreateFromBinaryOverlay(Path.Combine(data, m), SkyrimRelease.SkyrimSE)).ToList();
foreach (var mod in mods)
    foreach (var k in mod.Keywords)
        keywordNames[k.FormKey] = k.EditorID ?? k.FormKey.ToString();

bool Interesting(string name) =>
    name.StartsWith("ActorType") || name is "Vampire" or "DLC1Vampire" or "IsBeastRace" or "CreatureTypeAnimal" ||
    name.Contains("Werewolf") || name.Contains("VampireLord") || name.StartsWith("DLC1VampireBeast");

Console.WriteLine("== ActorType keywords defined");
foreach (var (key, name) in keywordNames.Where(k => k.Value.StartsWith("ActorType")).OrderBy(k => k.Value))
    Console.WriteLine($"  {name} {key}");

// Last definition of each race wins, like the game's load order.
var races = new Dictionary<FormKey, IRaceGetter>();
foreach (var mod in mods)
    foreach (var race in mod.Races)
        races[race.FormKey] = race;

Console.WriteLine("\n== Races (EditorID: keywords)");
foreach (var race in races.Values.OrderBy(r => r.EditorID))
{
    var kws = race.Keywords?.Select(k => keywordNames.TryGetValue(k.FormKey, out var n) ? n : k.FormKey.ToString()).Where(Interesting).OrderBy(n => n).ToList() ?? new();
    Console.WriteLine($"  {race.EditorID}: {string.Join(", ", kws)}");
}

Console.WriteLine("\n== ActorType keywords put directly on NPC records (count, examples)");
var npcKeywords = new Dictionary<string, List<string>>();
foreach (var mod in mods)
    foreach (var npc in mod.Npcs)
        foreach (var k in npc.Keywords ?? Enumerable.Empty<IFormLinkGetter<IKeywordGetter>>())
            if (keywordNames.TryGetValue(k.FormKey, out var n) && Interesting(n))
            {
                if (!npcKeywords.TryGetValue(n, out var list))
                    npcKeywords[n] = list = new();
                list.Add(npc.EditorID ?? npc.FormKey.ToString());
            }
foreach (var (name, list) in npcKeywords.OrderBy(k => k.Key))
    Console.WriteLine($"  {name}: {list.Count} ({string.Join(", ", list.Distinct().Take(6))})");

if (args.Contains("--dragons"))
    Dragons.Dump(mods);

if (args.Contains("--where"))
    foreach (var mod in mods)
        foreach (var r in mod.Races.Where(r => r.EditorID is "SprigganRace" or "SprigganMatronRace" or "SprigganEarthMotherRace" or "DLC2SprigganBurntRace" or "HorseRace" or "CartHorseRace" or "GiantRace" or "C00GiantOutsideWhiterunRace" or "DLC2GhostFrostGiantRace"))
            Console.WriteLine($"  {r.EditorID} {r.FormKey} (in {mod.ModKey})");

if (args.Contains("--skeletons"))
    Skeletons.Dump(mods);
