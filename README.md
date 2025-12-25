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
* **HP/SP Value Persistence:** Fixed an issue where current HP/SP values were incorrectly affected by changes to Max HP/SP (such as stat upgrades or equipment changes).
* **GM Kill Command:** Updated the `/kill` command to properly zero out the target's HP.
* **Messenger Deletion (Offline):** Fixed an issue where a companion needed to be online for a "remove from messenger" action to complete.
* **Messenger Deletion (Cross-Channel):** Fixed an issue where the removal message was visible to both parties even if they were in different channels or cores at the time of deletion.
* **Dungeon Party Logic:** Fixed an issue where the same message would popup to all affected parties when a leader tried to kick a player or a player tried to leave a team while inside a dungeon. Includes several other update message optimizations and dungeon logic improvements regarding party kicking/leaving.

### ⬆️ Feature Improvements
* **Job-Specific Stat Resets:** Individual stats reset scrolls (Items 71103, 71104, 71105, 71106) now recover stats to their initial values based on character job instead of defaulting to 1, returning the appropriate points. Translations now dynamically display the selected stat's value. Translation for Reset all-status scroll (71002) also adjusted from printing 'to 1'.
* **Logout Interruption:** Using a skill now automatically cancels the logout countdown for both the user and the target (if applicable).
* **Auto Potion Logic:** Auto potions can now be moved within the inventory while active and are automatically disabled immediately before being dropped to the ground.


