# Server Sources Repository

Clean server sources for educational purposes.

It builds as it is, without external dependencies.

## How to build

> cmake -S . -B build
>
> cmake --build build

---

## ✨ Key Updates and Fixes

The following changes have been implemented in the source code and build automation:

### ⚙️ Build Automation & Deployment

* **NEW Script: `dist2loc.py`:** Added a script in the sources root directory for **automated transferring of the newly compiled binaries** (executables) to their respected game server locations, as well as assigning necessary permissions.
    * ⚠️ **Action Required: Configuration**
        You **must** edit the `dist2loc.py` script to define the correct paths for your compilation output (`BUILD_BIN_DIR`) and your game installation root (`GAME_ROOT_DIR`) before running it.

### 🔒 Error fixing

* **Fix: Login Phase Packet Handling:** Fixed the **packet header 100** syserr message (`#define FIX_HEADER_CG_MARK_LOGIN`).

