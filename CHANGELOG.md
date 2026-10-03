# Changelog

## 1.0.0 (2026-10-04)

First release. Native SKSE port of Stances - Dynamic Animation Sets (Add-On) 1.13 by Ashen to Stances NG.

- Bonuses and penalties for the Bear, Wolf and Hawk stances (attack and power attack damage, damage taken, attack
  speed, movement speed, stamina regeneration, block, stagger, power attack stamina), every value a slider in Stances
  NG's own page in FLICK. Penalties to attack speed, movement speed and attack damage are capped at 90%.
- Attack damage bonus against 13 enemy types per stance. An enemy of several types gets only the largest of its
  bonuses (on a tie, the more specific type). Races of other mods are recognised at startup by their keywords,
  skeleton and, for frost dragons, frost resistance. Defaults: Bear +10% against spriggans, dwarven automatons,
  giants and trolls; Wolf +25% against undead and ghosts; Hawk +25% against people and creatures.
- Stance indicator: a diamond badge with the stance's icon and name pinned to the HUD; position and size sliders,
  shown only with weapons drawn by default, hidden in menus and with the HUD off.
- Options, on by default: Only with melee weapons, Revert Stance, Keep Neutral while reverted, Player Base Attack
  Speed, Hide Stances NG switch effects. Off by default: Debug log.
- English and Russian; Russian is used only when the FLICK font can show Cyrillic.
