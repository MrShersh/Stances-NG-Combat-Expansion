# Stances NG - Combat Expansion

**Make every stance a real choice.**

Stances NG - Combat Expansion gives the Bear, Wolf and Hawk stances of
[Stances NG](https://www.nexusmods.com/skyrimspecialedition/mods/117986) their own strengths and weaknesses, adds bonus
damage against chosen enemy types and shows your current stance on screen. Every value is a slider in Stances NG's own
page in [FLICK](https://www.nexusmods.com/skyrimspecialedition/mods/181603).

A native SKSE port of [Stances - Dynamic Animation Sets (Add-On)](https://www.nexusmods.com/skyrimspecialedition/mods/41251)
by Ashen: no Papyrus, no MCM.

## The stances

| Stance | Default modifiers |
|---|---|
| **Bear** | Attack damage +30%, power attack damage +10%, damage taken +10%, attack speed -15%, movement speed -10, stamina regeneration -10% |
| **Wolf** | Attack damage -10%, block +10%, stagger taken -10%, damage taken -5% |
| **Hawk** | Attack damage -20%, power attack stamina -20%, attack speed +20%, movement speed +10, stamina regeneration +10% |
| **Neutral** | none |

Every number is a slider; 0 turns that effect off. By default the stances work only with a melee weapon, a shield or
bare fists in hand. Spells are never affected.

## Damage vs enemy types

Each stance can hit harder against the enemies it is made for: 13 enemy types, one slider per type in every stance.
Defaults: Bear +10% against spriggans, dwarven automatons, giants and trolls; Wolf +25% against undead and ghosts;
Hawk +25% against people and creatures.

- **One bonus per enemy.** An enemy of several types (a vampire is a person *and* undead) gets only the largest of its
  bonuses; on a tie, the more specific type counts.
- **Works with creature mods.** Races from other mods are recognised at startup by their keywords and skeleton, no
  patches needed.

## Stance indicator

A small diamond badge with the icon and name of your stance, pinned to the HUD (bottom left by default). Position and
size are sliders. Hidden in menus and when the HUD is off; shown only with weapons drawn by default, can also be hidden
in Neutral.

## More options

- **Only with melee weapons** (on): no stance bonuses or penalties with a bow, a crossbow, a staff or only spells in hand.
- **Revert Stance** (on): Neutral while a bow, a crossbow or only spells and staves are in hand; the stance comes back
  with fists, a melee weapon or a shield.
- **Keep Neutral while reverted** (on): stance hotkeys do nothing while Revert Stance holds you in Neutral; a stance
  picked meanwhile is applied once a melee weapon is back in hand.
- **Player Base Attack Speed** (on): attack speed changes need a base WeaponSpeedMult above zero. If you use Attack
  Speed Framework or another mod that already provides it, turn this off, otherwise attacks become twice as fast.
- **Hide Stances NG switch effects** (on): no icon above the head and no body shader when switching.
- **Debug log** (off): stance changes, applied values and equipment checks go to the log.
- **Reset**: restores the default bonuses and options; the indicator layout is kept.

Penalties to attack speed, movement speed and attack damage are capped at 90%.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [Stances NG](https://www.nexusmods.com/skyrimspecialedition/mods/117986) 2.x
- [FLICK - Fuzz's Legally Intelligible Core Kit](https://www.nexusmods.com/skyrimspecialedition/mods/181603) for the
  settings page and the indicator. Without FLICK the bonuses still work, using the values in `StancesNG.toml`.

## Installation

1. Install with your mod manager and enable `Stances NG - Combat Expansion.esp` (light plugin, loads after
   `StancesNG.esp`).
2. In game, open FLICK (F7 by default) and go to the Stances NG page. This mod adds its own section there.
3. Settings are saved in `SKSE/Plugins/StancesNG.toml` (section `[CombatExpansion]`), next to the Stances NG settings.

Stances NG is not modified and none of its files are overwritten. English and Russian are included; Russian is used
only if the FLICK font can show Cyrillic (FLICK: `SETTINGS > Styles > Typeface > Jost-Regular.ttf`).

Bug reports: turn on **Debug log**, reproduce the problem and attach
`Documents/My Games/Skyrim Special Edition/SKSE/StancesNGCombatExpansion.log`.

## Building

CMake + vcpkg + MSVC 2022, CommonLibSSE-NG as a submodule (`git submodule update --init --recursive`). Put your local
paths into a `CMakeUserPresets.json` preset that inherits `vs2022` (`VCPKG_ROOT` in `environment`, optional
`DIST_DIR` = mod folder the build is copied to), then:

```
cmake --preset <your preset>
cmake --build --preset <your build preset>
```

- `tools/esp` builds `data/Stances NG - Combat Expansion.esp` with Mutagen
  (`dotnet run -- "..\..\data\Stances NG - Combat Expansion.esp"`, check with `dotnet run -- verify <esp>`). The DLL
  relies on its FormIDs; new records go only at the end.
- `tools/make-translations.ps1` generates `data/Interface/Translations` from `tools/translations.tsv`.
- `tools/icons` builds the indicator icons from `art/icons`.

## Credits

- **Driire** for brainstorming, testing and video footage.
- **Sigerious** for brainstorming, help with communication and media.
- **My girlfriend Marina** for her support, you're a real lifesaver and for making the YouTube video ❤
- **Ashen** (AshenShugarII) for [Stances - Dynamic Animation Sets (Add-On)](https://www.nexusmods.com/skyrimspecialedition/mods/41251).
- **aljo** and **Styyx** for [Stances NG](https://www.nexusmods.com/skyrimspecialedition/mods/117986).
- **OmecaOne** for the original Stances - Dynamic Animation Sets, where it all started.
- **Fuzzles** and **powerofthree** for [FLICK](https://www.nexusmods.com/skyrimspecialedition/mods/181603).
- **renketsu0** (WeaponSpeedMult guide) and **HaVeNII7** (Revert Stance idea), credited in the original add-on.
- **CharmedBaron** and contributors for CommonLibSSE-NG.

No files or assets from Stances NG, the original add-on or OmecaOne's mod are included. Code, plugin, icons and
artwork were made for this mod.

## License

GPL-3.0-or-later.
