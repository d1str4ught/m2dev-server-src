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
* **Login:** Fixed an issue where the login phase did not support the updated packet header `100` (`GC_MARK_LOGIN`).
* **Core Stability:** Implemented safeguards to prevent server cores from crashing (`core downer`) when a negative value is used in commands, such as `/m <vnum> <quantity>`.
* **Book reading in max level:** Reading skill books does not consume experience points anymore if the character has reached the maximum level (gets max level from game configuration, not a static number).
* **Character Selection:** Corrected the display of character stats and ensured gauge bars accurately reflect the character's stats during the selection phase.
* **Negative HP:** Ensured that a character's HP does not drop below 0 upon death.
* **CMakeLists for QC:** Target libraries link to `qc` (was `db`).

### ⬆️ Feature Improvements
* **All compiler warnings for building in FreeBSD environments have been fixed**
	* Types are synced in comparisons, dynamic string building and other functions and some definitions.
	* Minor performance improvements noticed in start time after changing `libthecore`'s `virtuals` into `atomic`.
	* More modern practices introduced in some parts of the codebase.
* **Messenger System:**
	* **Cross-Channel/Core Friend Requests:** Implemented support for sending and processing friend requests across different channels and server cores. `ChatPacket` notification is functional if the recipient is not found after searching all channels/cores.
	* **Live Status Updates:** Live updates for adding/removing friend for both parties.
	* **Request Guarding:** Added extensive validation checks to prevent:
		* Resending a request while a previous one is unanswered.
		* Sending a request to a person who has already sent an active, unanswered request to the sender.
		* Sending a friend request to an existing friend or to yourself.
		* All guard mechanisms provide translated `ChatPacket` notification messages and function correctly across channels and cores.
	* All pending friend requests are cleared on character disconnect (teleport, logout, kick, etc...).
	* A new friend request cancels all previous unanswered ones for that target.
* **Skill Cooldowns and States:**
	* **Level Reset Handling:** Skill cooldowns are now cleared upon a skill level reset or a skill-group reset. Works with `/setsk` setting horse skills levels to 0 as well.
	* **Togglable Skills:** If a togglable skill is reset, it is automatically deactivated, and its active effect is immediately removed from the character.
	* **Combo Skills:** Combo skill is automatically deactivate if its level is changed to 0 (`/setsk`).
* **.gitignore file:** Ignoring all files and directories ending in `_BAK` or `.BAK` (case-insensitive)


