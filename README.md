# m2dev-server-src
[![build](https://github.com/d1str4ught/m2dev-server-src/actions/workflows/main.yml/badge.svg)](https://github.com/d1str4ught/m2dev-server-src/actions/workflows/main.yml)


Clean server sources for educational purposes.

It builds as it is, without external dependencies.



## How to build

> mkdir build
>
> cd build
>
> cmake ..
>
> cmake --build .

---

## 📋 Changelog

### 🐛 Bug Fixes
* **Horse skill grade check:** Fixed a check that required the horse skill's grade to be exactly 3 (Grand Master) in order to use skills, leaving GMs that set their skill to P unable to use riding skills
* **Quest state loading:** Fixed an issue where quest states (`when login begin`) was being loaded before character affects on `PHASE_GAME`
* **Sura's Flame Ghost mounting:** Fixed Sura's Flame Ghost when mounting. The skill now damages nearby enemies when the horse skill is more than Level 10.
* **Experience points from chests:** Fixed a bug where experience points were being replaced by Experience Rings when opening chests
* **Gold from chests:** Fixed a bug where Gold was being replaced by a Gold inventory item that had no value when opening chests

### ⬆️ Feature Improvements
* **Nemere's Watchtower dungeon safeguards:** Added checks for automatically dismounting/prevent mounting of any kind when the character is inside the new Nemere's Watchtower dungeon
* **Conditional damage immunity system:** Added various checks for true per-hit conditional damage immunity with full control via Lua functions as well as clones of the default mob/group spawning Lua functions that return the monsters' VIDs for further manipulation from the quests. Exposed Lua functions below:
  * `d.regen_file_with_immunity`: Spawn all monster/groups from a dungeon folder's regen.txt file with conditional immunity embedded from spawn
  * `d.regen_file_with_vids`: The VIDs of all spawned monsters/groups from a dungeon folder's regen.txt file are being returned to Lua for further manipulation
  * `d.spawn_group_with_immunity`: Spawn a group of monsters via its ID with conditional immunity embedded from spawn
  * `d.spawn_group_with_vids`: The VIDs of all monsters from the group spawned are being returned to Lua for further manipulation
  * `d.spawn_mob_with_immunity`: Spawn a single monster with conditional immunity embedded from spawn
  * `npc.add_damage_immunity_condition`: Add a damage immunity condition to an already spawned monster
  * `npc.clear_damage_immunity_conditions`: Clear all damage immunity from a monster so it can take damage normally again
  * `npc.is_damage_immune`: Check if a mob has damage immunity using its VID
  * `npc.set_damage_immunity`: Set damage immunity to a monster using its VID
  * `npc.set_damage_immunity_with_conditions`: Set conditional damage immunity to a monster using its VID
  * **Immunity vs Conditional immunity**: When a monster is immune to damage all hits are returning as MISS and it cannot be poisoned, burned, slowed or stunned. A condition is a rule that when applied, the monster's immunity is being ignored (for example, a mob is immune to damage unless the attacker is a Ninja - job 1). Multiple conditions are possible.
  * More about the available conditions for damage immunity in `game/char.h`


