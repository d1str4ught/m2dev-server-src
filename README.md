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
* **Realtime Character Level Updates:** Implemented the reversed fix (credits to Mali) for updating character level in real-time across game view and all windows (such as Guild window) for all viewing players.

### ⬆️ Feature Improvements
* **Character position auto-reverts from ```POS_FIGHTING``` to ```POS_STANDING``` after 10 seconds of battle inactivity:** Affects the final logout countdown (from 10s to 3s) as character state updating for real-time data across server functions. Tests performed (and succeeded):
	* The character takes hits from another character.
	* The character hits another character.
	* The character takes hits from mobs.
	* The character hits mobs/stones.
	* The character uses an aggressive skill to another character.
	* An aggressive skill is being used on the character.
	* The character uses an aggressive skill to a mob/stone, without killing the instance.
	* A boss uses a skill on the character.

All credits go to #tw1x1 for this amazing and smoothly executed fix!


