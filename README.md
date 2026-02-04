# M2Dev Server Source
[![build](https://github.com/d1str4ught/m2dev-server-src/actions/workflows/main.yml/badge.svg)](https://github.com/d1str4ught/m2dev-server-src/actions/workflows/main.yml)


Clean server sources for educational purposes.

It builds as it is, without external dependencies.


**For more installation/configuration, check out the [instructions](#installationconfiguration) below.**


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

### Encryption & Security Overhaul

The entire legacy encryption system has been replaced with [libsodium](https://doc.libsodium.org/).

#### Removed Legacy Crypto
* **Crypto++ (cryptopp) vendor library** — Completely removed from the project
* **Panama cipher** (`CFilterEncoder`, `CFilterDecoder`) — Removed from `NetStream`
* **TEA encryption** (`tea.h`, `tea.cpp`) — Removed from both client and server
* **DH2 key exchange** (`cipher.h`, `cipher.cpp`) — Removed from `EterBase`
* **Camellia cipher** — Removed all references
* **`_IMPROVED_PACKET_ENCRYPTION_`** — Entire system removed (XTEA key scheduling, sequence encryption, key agreement)
* **`adwClientKey[4]`** — Removed from all packet structs (`TPacketCGLogin2`, `TPacketCGLogin3`, `TPacketGDAuthLogin`, `TPacketGDLoginByKey`, `TPacketLoginOnSetup`) and all associated code on both client and server
* **`LSS_SECURITY_KEY`** — Dead code removed (`"testtesttesttest"` hardcoded key, `GetSecurityKey()` function)

#### New Encryption System (libsodium)
* **X25519 key exchange** — `SecureCipher` class handles keypair generation and session key derivation via `crypto_kx_client_session_keys` / `crypto_kx_server_session_keys`
* **XChaCha20-Poly1305 AEAD** — Used for authenticated encryption of handshake tokens (key exchange, session tokens)
* **XChaCha20 stream cipher** — Used for in-place network buffer encryption via `EncryptInPlace()` / `DecryptInPlace()` (zero overhead, nonce-counter based replay prevention)
* **Challenge-response authentication** — HMAC-based (`crypto_auth`) verification during key exchange to prove shared secret derivation
* **New handshake protocol** — `HEADER_GC_KEY_CHALLENGE` / `HEADER_CG_KEY_RESPONSE` / `HEADER_GC_KEY_COMPLETE` packet flow for secure session establishment

#### Network Encryption Pipeline
* **Client send path** — Data is encrypted at queue time in `CNetworkStream::Send()` (prevents double-encryption on partial TCP sends)
* **Client receive path** — Data is decrypted immediately after `recv()` in `__RecvInternalBuffer()`, before being committed to the buffer
* **Server send path** — Data is encrypted in `DESC::Packet()` via `EncryptInPlace()` after encoding to the output buffer
* **Server receive path** — Newly received bytes are decrypted in `DESC::ProcessInput()` via `DecryptInPlace()` before buffer commit

#### Login Security Hardening
* **Removed plaintext login path** — `HEADER_CG_LOGIN` (direct password to game server) has been removed. All game server logins now require a login key obtained through the auth server (`HEADER_CG_LOGIN2` / `LoginByKey`)
* **CSPRNG login keys** — `CreateLoginKey()` now uses `randombytes_uniform()` (libsodium) instead of the non-cryptographic Xoshiro128PlusPlus PRNG
* **Single-use login keys** — Keys are consumed (removed from the map) immediately after successful authentication
* **Shorter key expiry** — Expired login keys are cleaned up after 15 seconds (down from 60 seconds). Orphaned keys (descriptor gone, never expired) are also cleaned up
* **Login rate limiting** — Per-IP tracking of failed login attempts. After 5 failures within 60 seconds, the IP is blocked with a `BLOCK` status and disconnected. Counter resets after cooldown or successful login
* **Removed Brazil password bypass** — The `LC_IsBrazil()` block that unconditionally disabled password verification has been removed

#### Pack File Encryption
* **libsodium-based pack encryption** — `PackLib` now uses XChaCha20-Poly1305 for pack file encryption, replacing the legacy Camellia/XTEA system
* **Secure key derivation** — Pack encryption keys are derived using `crypto_pwhash` (Argon2id)

<br>
<br>

---

<br>
<br>

# Installation/Configuration
This is the first (and perhaps the most complex) project that needs to be configured for a smooth system installation.

Below you will find a comprehensive guide on how to install and configure all the necessary components from scratch.

This guide is made using a **Windows** environment as the main environment. If you are not using Windows, please feel free to **adapt these instructions for your operating system**.

This guide also uses the latest versions for all software demonstrated as of its creation date at January 31, 2026.

© All copyrights reserved to the owners/developers of any third party software demonstrated in this guide other than this project/group of projects.

<br>

### 📋 Order of projects configuration
If one or more of the previous items is not yet configured please come back to this section after you complete their configuration steps.

>  - ▶️ [M2Dev Server Source](https://github.com/d1str4ught/m2dev-server-src)&nbsp;&nbsp;&nbsp;&nbsp;[**YOU ARE HERE**]
>  - ⏳ [M2Dev Server](https://github.com/d1str4ught/m2dev-server)
>  - ⏳ [M2Dev Client Source](https://github.com/d1str4ught/m2dev-client-src)
>  - ⏳ [M2Dev Client](https://github.com/d1str4ught/m2dev-client)&nbsp;&nbsp;&nbsp;&nbsp;[**ALSO CONTAINS ADDITIONAL INFORMATION FOR POST-INSTALLATION STEPS**]

<br>

### 🔀 Available options
The source and the server project can be ran in either a **FreeBSD** or a **Windows** environment. Other Linux flavors are not fully supported and may not be fully compatible yet.

This tutorial will be showing how to install in both FreeBSD and Windows environments.

<br>

## ![FreeBSD](https://metin2.download/picture/36rMX2LFRT3U8qlPNQ81nsPZ7F4yg4c3/.png) **FreeBSD**

### 🧱 Software Prerequisites

<details>
  <summary>
    Please make sure that you have installed the following software in your machine before continuing:
  </summary>

  <br>

  > <br>
  >
  >  - ![VirtualBox](https://metin2.download/picture/FyS88Eea68mf8t9L7gVZXG1tLSeIpm6q/.png)&nbsp;&nbsp;**VirtualBox**:&nbsp;&nbsp;You can also use **VMWare** or whatever Virtualization software you prefer, this guide will be covering the installation and use of VirtualBox. [Download](https://www.virtualbox.org/wiki/Downloads)
  >
  >  - ![WinSCP](https://metin2.download/picture/q2PUiqc5731ZfHkrzHvhvUUhI436y2l9/.png)&nbsp;&nbsp;**WinSCP**:&nbsp;&nbsp;You can also use **FileZilla** or any other FTP/SFTP software you prefer, this guide will be covering the installation and use of WinSCP. [Download](https://winscp.net/eng/download.php)
  >  - ![Navicat Premium Lite](https://metin2.download/picture/2dAmE5sX2U3VtErqhijVB3hRI57Z7yWp/.png)&nbsp;&nbsp;**Navicat Premium Lite (Free Version)**:&nbsp;&nbsp;You can also use the official **MySQL/MariaDB Workbench clients from Oracle**, or any other database management software you prefer, but most people in this space are more familiar with Navicat. [Download](https://www.navicat.com/en/download/navicat-premium-lite)
  >  - ![PuTTY](https://metin2.download/picture/I4Y1EDaGrl0D6Pj0alSjSbUo7okYd5yK/.png)&nbsp;&nbsp;**PuTTY (optional)**:&nbsp;&nbsp;Using an SSH/Telnet software can be a little more convenient than the defaut interface as it allows you to easily scroll, copy/paste content into the terminal and renders a little bit faster. Feel free to download alternatives if you are more familiar with one or even use the default VirtualBox VM interface to execute your commands. [Download](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html)
  >
  > <br>

</details>

<br>

### ▶️ Creating the Virtual Machine
First things first, a Virtual Machine is something like "a computer within your computer". You can install any operating system you wish in your virtual machine, could be macOS, Android, Linux, or even an old and forgotten Windows version (XP, 7, etc...).

<details>
  <summary>First things first</summary>

  <br>

  > <br>

  > It is very important to know if your machine is x32 (x86) or x64.
  >
  > Systems that are x32 cannot run x64 virtual machines. If your system is an x64, make sure you have the "**Virtualization**" settings to ON in your BIOS settings, otherwise the x64 virtual machine options won't appear in your interface when creating the VM.
  >
  >This tutorial will not be covering how to turn on virtualization as every BIOS differs from the rest although most modern PC's have this option ON by default, so you're probably fine. Here is how to check if your system is x32 or x64 (mine is x64):
  >
  > ![](https://metin2.download/picture/pya6AVw2sXfbsKdZ6cyPys39ST0s7VyD/.png)
  >
  > <br>

</details>

<br>

Next, we will be installing the **FreeBSD** operating system. To install an OS (whether it's in your physical PC or a VM) you need the "installer". This is gonna be an ISO file that contains the installation files. For our case you can download the FreeBSD ISO file like this:

<br>

<details>
  <summary>
    Downloading a FreeBSD ISO
  </summary>

<br>

  >
  ><br>
  >
  >1. Go to the [official FreeBSD website](https://www.freebsd.org/) and select "**Get FreeBSD**" from the navbar:
  >
  > ![](https://metin2.download/picture/1fcW7z32KV7i5SKfHwbnQqv3qUU3pYn1/.png)
  >
  > <br>
  >
  > 2. Scrolling down in that page, you will find sections for different versions. Go to your desired version and select **amd64** for an x64 installer, or **i386** for an x32 installer (not available in latest versions) **from the Installer tab (first one)**.
  >
  > ![](https://metin2.download/picture/InWxOo9hSax7Q173YosuRUgVg18TW4pl/.png)
  >
  > <br>
  >
  > 3. You will be redirected to the folder with the available downloads, here select the one that ends in "**XXX_disc1.iso**".
  >
  > ![](https://metin2.download/picture/HxMdIgLr0pYlT7cCfh0PJxvUChd0pWxR/.png)
  >
  > <br>
  >
  > 4. Grab a coffee ☕, may take its time to download depending on your connection speed...
  >
  > <br>
  >

</details>

<br>

You now have everything you need to create your virtual machine.

<br>

<details>
  <summary>
    Creating the VM
  </summary>

<br>

  >
  > <br>
  >
  > Open up your newly installed virtualization software and create a new virtual machine:
  >
  > ![](https://metin2.download/picture/eR4XZTM0XV2pUP1mnQOOWp0GIX5U5k31/.png)
  > Small note: if you just installed the program, your list of VMs will obviously be empty
  >
  > <br>
  >
  > You will now see the New VM wizard. Starting from the first step:
  >
  > 1. Add a VM Name. It can be whatever you want.
  >
  > 2. VM Folder is where the files of your new VM will be stored. You don't need to change that unless your current selection's drive doesn't have enough space (20-50 GB free)
  > 3. In ISO Image select your downloaded ISO file (the installer of the OS)
  > 4. In OS choose BSD (obviously)
  > 5. In OS Distribution, choose FreeBSD (duuh 😂)
  > 6. In OS Version choose 32-bit or 64-bit according to the ISO image you downloaded
  > 7. **Leave "Proceed with Unattended Installation" unchecked**
  >
  > <br>
  >
  > ![](https://metin2.download/picture/Crm3Kc0fgR8fPbsMUAe8mKHXrj4Q7HP0/.png)
  >
  > <br>
  >
  > Now, skip to the 3rd step, where you will set the number of RAM and CPU processors available to your VM:
  >
  > 1. RAM: FreeBSD can run with very low amounts of RAM consuption, however the higher the number, the more resources it has to compile the source. Choose the maximum number of MB of RAM you would like to allocate. **It is not recommended to go over the middle.** The number must be (**the GB you wanna allocate * 1024**). Here is a [free tool](https://www.gbmb.org/gb-to-mb) that can help with this calculation if you're too lazy to open up your calculator (copy the in binary number).
  >
  > 2. The max amount of CPU cores you would like to allocate. One is super enough for the VM to run however this is the resource that can truly make your builds even lightning fast, the more cores the faster the build.
  >
  > ![](https://metin2.download/picture/YHpU3IyXUzqDO117Ej8dkV3ZxD94g3Gg/.png)
  >
  > <br>
  >
  > In the final step you must define the maximum disk space your VM's "hard drive" can occupy:
  >
  > 1. Select the virtual hard drive's space. In my case I will pick 64 GB so I can have room for various builds
  >
  > 2. Leave Hard Disk File Type and Format to VDI
  > 3. It is recommended to leave the "Pre-allocate Full Size" option unchecked. By checking it you will occupy the selected GB of space in your hard drive and the space will be reserved only for your VM. Leaving it off will ensure that the VDI will only occupy its actual size within your physical drive.
  > 4. That's it! Click **Finish**!
  >
  > ![](https://metin2.download/picture/Qv4895V0565qJF2Nefqkn31xU73y9dn7/.png)
  >
  > <br>
  >

</details>

<br>

### ⚙️ Configuring your Virtual Machine
Before diving in to the installation let's tweak some additional settings that are required for the connectivity of the VM.

<br>

<details>
  <summary>
    Configuration
  </summary>

  <br>

  > 
  > <br>
  >
  > With your VM selected in the main interface, click on **Settings**
  >
  > ![](https://metin2.download/picture/FgbE2FPcOh5aic646e38kUX7aiNz0NT3/.png)
  > Scroll down to the Network section and make sure that:
  >
  > 1. **Enable Network Adapter** is checked in the Adapter 1 tab
  > 2. Set **Attached to** to **Bridged Adapter**
  > 3. In **Name** you have a few different options depending on your device's connection type and the names will differ according to the manufacturer of your machine and the parts they used to build it:
  > > 1. **\<anything> Family Controller**: this is for Ethernet connections
  > > 2. **\<anything> WiFi/Wireless/WLAN**: this is for WiFi connections
  > > 3. **Remote NDIS Compatible Device**: this is for USB-tethered hotspot from a smartphone
  > > 4. Anything else is probably virtual network drivers, likely from VPN software, ignore those.
  > 4. **Adapter Type**: the name of the device used to connect, usually the default selection is ok...
  > 5. Leave the rest to their **default** values.
  > 6. Click **OK**.
  >
  > ![](https://metin2.download/picture/ZART9pUge663nLOMZc33R92b06p9Mydr/.png)
  > 
  > <br>
  >
</details>

<br>

### 💿 Installing the OS
Start your new VM by double clicking it in the VM list of your main interface or by clicking the Start button with your VM selected.

<br>

<details>
  <summary>
    Installation
  </summary>

  <br>

  >
  > <br>
  >
  > The boot countdown will start, optionally you can press ENTER to skip it. Wait until you see the blue screen:
  >
  > ![](https://metin2.download/picture/8ibHz9GqLVKXK9sTJQmZQI9wzDb8SOx3/.png)
  >
  > <br>
  >
  > Instructions for navigating in the installer:
  >
  >  - **ENTER**: confirm option (hilighted with blue color)
  >  - **TAB**: go to the next option
  >  - **Arrow keys**: move left or right in selections like the one above, or up and down in vertical list selections like the next image
  >  - **SPACE**: Activates or deactivates the option (works like a checkbox) for available options:
  > >  - **[&nbsp;&nbsp;]**: deactivated state, press SPACE while you have the option selected to activate it
  > >  - **[X]**: activated state, press SPACE while you have the option selected to deactivate it
  > >  - **(&nbsp;&nbsp;)**: deactivated state, press SPACE while you have the option selected to activate it, **ONLY ONE SELECTION IS AVAILABLE**
  > >  - **(X)**: activated state, press SPACE while you have the option selected to deactivate it, **ONLY ONE SELECTION IS AVAILABLE**
  >
  > <br>
  >
  > Select **Install** to continue (ENTER)
  >
  > In the next screen you will need to choose your keyboard layout. You can only select one. Usually the default (US) is fine but it is recommended to use your language. Press ENTER on the first option to confirm your selection and move to the next step.
  >
  > ![](https://metin2.download/picture/uiAR525TNC1xBkkIKcg7PFAt6RqXwvWo/.png)
  >
  > <br>
  >
  > Now set up a hostname. It can be whatever you want.
  >
  > ![](https://metin2.download/picture/331lYi57675wksaZxjzGW7YRpeqt77hp/.png)
  >
  > <br>
  >
  > #### **FreeBSD 15.0-RELEASE and future versions**
  >This next step is about the method of the system dependencies installation. So far FreeBSD had the system packages updated via the freebsd-update command. From 15.0 and later you have the option to use **pkg** to install these dependencies. The traditional method will be removed in FreeBSD 16.0-RELEASE. This option will not be available if you are installing FREEBSD 14.3 or lower. I will proceed with the new option (**Packages (Tech Preview)**) for this VM:
  >
  > ![](https://metin2.download/picture/D5SCyihVQ6OUX1om42y3z8ZaM3VfzxaB/.png)
  >
  > <br>
  >
  > If you selected the new packages installation option you will be prompted to select whether you wanna install via the online method (Network) or the "offline installer" method (Offline (Limited Packages)). If you have configured your VM's network settings correctly you will be able to use the Network option, otherwise proceed with the Offline method and continue the package installation once your OS installation is finished and you have established internet connection to your VM.
  >
  > <br>
  >
  > #### **All versions (continuiing)**
  > **Network configuration**
  >
  > If you are installing FreeBSD 14.3-RELEASE or lower version this step might show up later. This step should give you the option to select a network interface.
  >
  > **Auto vs Manual**: select Auto for automatically obtaining a DHCP address from your router or select Manual to setup a static IP address that will never change (requires further configuration).
  >
  > ![](https://metin2.download/picture/RgWmD2NCH0cdyp1uwEeayOY3M20f0qw3/.png)
  >
  > <br>
  >
  > **Disk partitioning**
  >
  > To keep this guide short, the difference between these options will not be explained here.
  >
  > Simply pick whatever the default is for your selected version. In some older versions, UFS is the default because ZFS was still experimental while in newer versions like 14.3-RELEASE and above, ZFS is the new default standard.
  >
  > Once the content of the second image appears, hit ENTER one more time to accept the default configuration.
  >
  > 1. 
  > ![](https://metin2.download/picture/MdaKOqQYXeYcyTF5900MgFL4lez9v7YU/.png)
  >
  > 2. 
  > ![](https://metin2.download/picture/N4tNm80xb1fT3tizG8xJ7tzg0zK7k8lV/.png)
  >
  > Continuing with disk partitioning, select the **No Redundancy** option in striping as you only have one disk.
  >
  > In the second screen, press SPACE to select your disk and then ENTER. If you accidentally press ENTER without selecting the disk, press ENTER again to close the error dialog and make the content in the second image appear again.
  >
  > 1. 
  >
  > ![](https://metin2.download/picture/7IkyTgKMXuIayuY2f9jSLlO33qCFeVWG/.png)
  >
  > 2. 
  >
  > ![](https://metin2.download/picture/1lY2AUp5bfz7u8ccGT5I59aF88vk97Tk/.png)
  >
  > A final confirmation dialog will appear. Use the arrow keys and ENTER to confirm **YES**.
  >
  > ![](https://metin2.download/picture/2vKgC40py7tg98m6T3AQm2eEi7AgLNaj/.png)
  >
  > Note: this may take a while as your VM will now download and install packages.
  >
  > <br>
  >
  > Next you will have to select the components to be installed in the OS. The default options are fine, however, **here is a big difference between 14.3-RELEASE and lower vs 15.0-RELEASE**:
  >
  > **FreeBSD lower than 15.0-RELEASE**: In versions before 15.0 there is a "**ports**" option (deselected by default) which installs the ports tree (/usr/ports). Before 14.0-RELEASE, we had portsnap for the ports tree. Once 14.0-RELEASE rolled out, the new standard became cloning the ports in /usr/ports from the official Github repo. Enabling **ports** will pre-install the ports tree index and that will save you the step of manually downloading it once the installation is complete. Using ports is **NOT** required for your complete setup but it is an option that will be explained further along the way.
  >
  > **FreeBSD 15.0-RELEASE**: From this new distribution forward the ports option is no longer available. We will use **pkg** to install Git, and Git to install ports (if you are going for ports) for this version. One very useful thing in 15.0 is the **devel** option, which pre-installs Clang and LLVM, the biggest, most time consuming port to build (if going with ports). This guide will continue with the **devel** option selected so that these packages get pre-installed along with the system, instead of installing it manually later.
  >
  > You may leave the rest to their default selection states and hit ENTER.
  >
  > ![](https://metin2.download/picture/N9tTplh2v6fc29TfiiuNZ5r2ugffhrua/.png)
  >
  > It will now start installing everything via **pkg** if you selected the **Packages option**, or the classic way if you selected **Distribution Sets**.
  >
  > <br>
  >
  > Next, it's time to set your root password. In versions lower than 15.0, it will be in terminal style. In version 15.0, it will look like the picture:
  >
  > ![](https://metin2.download/picture/2eLNeE9UZG6zEZx78RRJWVWrJEUwIV4q/.png)
  >
  > Since this is probably a VM and not a VPS, feel free to setup something small and easy for convenience.
  >
  > **Use strong passwords if you are setting up a server that is accessible from multiple IP's**!
  >
  > You can also skip root password (if installing a local, development instance) but it is **not recommended**.
  >
  > Use the TAB or arrow keys to move between the text fields and ENTER to confirm your selection.
  >
  > <br>
  >
  > **Setting up the timezone**
  >
  > Select your desired timezone:
  >
  > 1. 
  >
  > > ![](https://metin2.download/picture/anzUYsgscAq3NLj20tKyFm7HWG9LncYj/.png)
  >
  > 2. 
  >
  > > ![](https://metin2.download/picture/owXNK91NyGJD87pv2T0HU6dBth1u8Ta9/.png)
  >
  > A confirmation like the one in the image will appear, simply confirm your selection:
  >
  > ![](https://metin2.download/picture/l6s6tb2bwDLVKR5Txnw744ZIq3V2wob7/.png)
  >
  > Adjust the date and time if you need to using the arrow keys (left/right = selection, up/down = change values), or simply skip these steps.
  >
  > 1. 
  >
  > > ![](https://metin2.download/picture/ibGv8vn2c9atGufC4S1h0zw3K9g9Tk2j/.png)
  >
  > 2. 
  >
  > > ![](https://metin2.download/picture/m7s85YE7l8lKUHPgGvl8u2f3EkFAmcsO/.png)
  >
  > <br>
  >
  > **Some final touches**
  >
  > In the next 2 steps, accept the defaults. Next, wait for the countdown to end or hit ENTER.
  >
  > 1. 
  >
  > > ![](https://metin2.download/picture/E0065sOUQtPR3Ix34i31m4B3ZqQ2C3vP/.png)
  >
  > 2. 
  > > ![](https://metin2.download/picture/mTDNfYbHYN4GMT9uD2yHPt5592AfU8gI/.png)
  >
  > The system will now prompt you to add additional users. You don't need any, but feel free to add if you wish, if not, just confirm **No**.
  >
  > ![](https://metin2.download/picture/EoPAbJ0b4kciv4VMHiXgk94YDbTC634l/.png)
  >
  > In case you accidentally selected Yes and you don't want to create an additional user, simply type something whatever in the username and then keep hitting ENTER until you reach the final question, there **just type N and hit ENTER to abort user creation. Then hit n and ENTER again**.
  >
  > <br>
  >
  > **Finalizing the installation**
  >
  > Select **Finish** once the screen from the first image pops up and select the default **No** once the second one does unless you wanna type a few final commands for modifying the installation further.
  >
  > 1. 
  >
  > > ![](https://metin2.download/picture/qCGw5e1GqXS19oScR2a1hH7DouY80z5s/.png)
  >
  > 2. 
  > > ![](https://metin2.download/picture/dXYepUJH7izzzbaSOR9MJY1p2VgZKVH1/.png)
  >
  > After that, select **Reboot**.
  >
  > ![](https://metin2.download/picture/eyzc9JUe2cuSGck06ib16erHocmuculg/.png)
  >
  > **VERY IMPORTANT**: After hitting ENTER wait for the system to shutdown and immediately **right click on the disc icon at the bottom right corner (second icon to the left)**, then click **Remove Disk From Virtual Machine (last option)**. You need to **do this before the system boots** otherwise the installation will start again.
  >
  > ![](https://metin2.download/picture/PJKM5R2UneG9wWIs0UeGg33feJ7ST9O1/.png)
  >
  > Press **ENTER** on the boot countdown and once prompted, enter root and your selected password.
  >
  > <br>
  >
  > ### ✅ The installation is complete!
  >
  > <br>
  >
  > For a more in-depth guide to install **FreeBSD 15.0-RELEASE**, you can watch [this video](https://www.youtube.com/watch?v=JSTFpe70tOo):
  >
  > **(DO NOT FOLLOW THE INSTRUCTIONS FOR THE INSTALLATION MEDIA AT THE BEGINNING, create a virtual machine instead like this guide explains)**
  >
  > For a more in-depth guide to install **FreeBSD 14.3-RELEASE**, you can watch [this video](https://www.youtube.com/watch?v=_kVbERKwtvk):
  >
  > **(DO NOT FOLLOW THE INSTRUCTIONS FOR THE INSTALLATION MEDIA AT THE BEGINNING, create a virtual machine instead like this guide explains)**
  >
  > <br>
  >
</details>

<br>

### ⚙️ Configuring the OS
After entering your **root** username and password into the terminal, the first thing we need to do is ensure that external software from your host OS can connect as **root** to your new VM, such as PuTTY or WinSCP.

You can skip this step if you are gonna be working with a non-root user, but for a local VM it is recommended to work as **root**.

First things first, you need to know your VM's internal IP, the IP assigned to your instance by the DHCP server during the first time you connected it to the internet.

<details>
  <summary>
    Acquiring the IP
  </summary>

<br>

  >
  > <br>
  >
  > Type `ifconfig` in the terminal like this, and hit ENTER:
  >
  > ![](https://metin2.download/picture/Gr4u2v1Xtdn84E26ax09VI1Nu9Jv5d47/.png)
  >
  > If your VM is connected to the internet, you should be seeing something like this:
  >
  > ![](https://metin2.download/picture/vZ3DsTN4U4Qhmh0F55AtjCmtj19Z4Uf1/.png)
  >
  > Your IP should be printed in the `inet` section of your network's interface like the image highlights.
  >
  > If you selected `Family Controller` or `WiFi/WLAN/Wireless` in the VM's netwrork settings, your IP most probably starts with `192.168...` instead of `10...`
  >
  > **Copy this value from your VM's terminal and write/type it down somewhere, you will be needing it in the next steps.**
  >
  > <br>
  >
</details>

<br>

<details>
  <summary>
    Enabling external root access
  </summary>

  <br>

  >
  > <br>
  >
  > In your VM's terminal, type ``ee /etc/ssh/sshd_config`` to edit the sshd configuration file.
  >
  > ![](https://metin2.download/picture/86H6wVAKghE0GP8R1oE5umrO0UfT0evl/.png)
  >
  > Use the ARROW KEYS to navigate through the editor between the different characters and words.
  >
  > Keep going down until you find this line: `#PermitRootLogin no` and change it to `PermitRootLogin yes` (remove the `#` character as well):
  >
  > ![](https://metin2.download/picture/p5sfEy44PXvT8Wj8EDqgKnlcF760FBPo/.png) ![](https://metin2.download/picture/A0fEawSU8JRnl3o7D25ghfvS5de3Lc3u/.png)
  >
  > <br>
  >
  > Next, keep scrolling down until you find `#UseDNS yes` and change it to `UseDNS no` (remove the `#` character as well):
  >
  > ![](https://metin2.download/picture/T21crsQb6w8032gc07w12nGblG4sgcqr/.png)
  >
  > <br>
  >
  > Done! Press the `esc` key to exit the editor, and with option `a) save changes` selected, press ENTER to exit:
  >
  > ![](https://metin2.download/picture/7Va4AIGn0QcIZFx7gVZphG75A2xy26oM/.png) ![](https://metin2.download/picture/T8E5A7tVFvaNKjmUnGqXh279D0M03X95/.png)
  >
  > Finally, restart the SSHD service by executing this command:
  >
  > ```
  > serivice sshd restart
  > ```
  > <br>
  >
  > ### ✅ Remote root access is now enabled!
  >
  > <br>
  >
</details>

<br>

### 🛜 Connecting with third party software

This guide will now be showing you how to connect as root from your host OS with PuTTY and WinSCP.

Connecting with Navicat (or your database management software) is not yet possible as we have not yet installed MySQL/MariaDB, once we install the packages, this guide will show how to do that as well.

<details>
  <summary>
    Connecting with PuTTY/SSH software
  </summary>

<br>

  >
  > <br>
  >
  > > **NOTE**: You can also use your Windows Command Prompt (CMD/Powershell) to remotely connect to your VM's terminal
  >
  > <br>
  >
  > ### PuTTY/SSH software
  >
  > Fire up PuTTY. You should be seeing this screen (your list will obviously be empty if you've never done this before):
  >
  > ![](https://metin2.download/picture/8by0SrlW1drcXuS0U8hjMOWexaU7oqTE/.png)
  >
  > <br>
  >
  > Configuration:
  >
  > 1. **Host Name (or IP address)**: The IP address you copied from the output of the  `ifconfig` command in the terminal.
  > 2. **Port**: Leave that to `22`.
  > 3. **Connection type**: Leave that to `SSH`.
  > 4. **Optionally** you can type a custom name in **Saved Sessions** and click **Save** so you can easily load your saved session by double-clicking it in the Saved Sessions list, or by selecting it and clicking **Load**.
  > 5. Once you're done with your setup, click **Open** to open up your new session for the first time.
  >
  > ![](https://metin2.download/picture/q00mv1Os5ilu1MjPlEiK1Zrb52CSNyw9/.png)
  >
  > <br>
  >
  > If your IP, port and connection type are correct and your VM has internet access, you should be seeing this along side with your new terminal window:
  >
  > ![](https://metin2.download/picture/4Hn2Kujw6RNPlO9CfyCMnTdt08gtKwAd/.png)
  >
  > This dialog appears in every newly created session. Simply hit **Accept** to disable it from ever appearing again for this session.
  >
  > You can also continue by clicking **Connect Once** but with this option, you will see this dialog again the next time you attempt to connect to your session.
  >
  > <br>
  >
  > Type in your **root** username (root) and password and hit ENTER.
  >
  > <br>
  >
  > ### ✅ You have successfully connected to your VM using a third party SSH software!
  >
  > <br>
  >
  > ### Windows Command Prompt (CMD/Powershell)
  >
  > Fire up your command prompt in any directory and type `ssh root@<your_ip_address>` and hit ENTER:
  >
  > ![](https://metin2.download/picture/kfWmQo6BzIy93KaUn2hib7B2OmX223rD/.png)
  >
  > Type in your password and you're in!
  >
  > <br>
  >
  > ### ✅ You have successfully connected to your VM using CMD/Powershell!
  >
  > <br>
  >
</details>

<br>

Now let's connect with an FTP/SFTP software like WinSCP.

This kind of software will allow you to see your files and folders within your VM in a more user-friendly and familiar layout. Think of it as your Windows File Explorer, but for your VM.

<details>
  <summary>
    Connecting with WinSCP/SFTP software
  </summary>

<br>

  >
  > <br>
  >
  > Fire up WinSCP (or your FTP/SFTP software) and make sure that **New Site** is selected at the left-side panel:
  >
  > 1. **Host name**: Your IP address from the output of the `ifconfig` command.
  > 2. **Port number**: Leave that one to `22`.
  > 3. **User name**: Here make sure that `root` is your selected username.
  > 4. **Passsord**: Your root password (obviously).
  > 5. **Done**! You can now either hit **Login** to connect straight-up, or **Save** to save your Site at the list in the left-side panel.
  >
  > ![](https://metin2.download/picture/Lpvb0TY0raa0978r1Xr1dO0VSxAcxtqn/.png)
  >
  > <br>
  >
  > If you clicked **Save** you should be seeing this little additional dialog:
  >
  > ![](https://metin2.download/picture/j60Wa6ovtCWZe7F0g977xrC4Mun3shR4/.png)
  >
  > Here:
  > 1. In **Site name** give a custom, meaningful (for you) name to your new site.
  > 2. You can enable **Save password (not recommended)** for your convenience **if your VM is a local instance!** It is strongly recommended **NOT** to do this for VPS/remotely hosted machines.
  > 3. Click **OK** to save your site.
  >
  > When the dialog closes, you can connect for the first time by clicking the **Login** button.
  >
  > <br>
  >
  > Like with PuTTY, during your first connection to the VM, the program is going to show this dialog:
  >
  > ![](https://metin2.download/picture/1RjAK7JoU71v6HZDZD8767bD62s0RhE8/.png)
  >
  > Just click **Accept** to save this site in your whitelist. After that, this dialog will not appear again for this IP address.
  >
  > <br>
  >
  > ### ✅ You have successfully connected to your VM with your FTP/SFTP software!
  >
  > <br>
  >
</details>

<br>

### ⬇️ Installing packages/ports

As mentioned during the installation, FreeBSD works with packages/ports for accessing third-party software within the system.

You can think of packages and ports like apps on your phone, the system installs them and uses them to show or/and process new information.

The difference between **ports** and **pkg** is that pkg works like your phone's AppStore or Google Play Store, it simply downloads and installs apps (packages), or you can think of it as downloading an MSI installer from a website and running it.

**Building with ports can be extremely slow compared to pkg but offers more customization.**

**ports** from the other hand downloads the source code of the package and compiles it right inside your system. This option is useful for components you wish to edit their code before installing them, or adjust the build configuration, etc...

<details>
  <summary>
    How to setup pkg and use it to install third-party software
  </summary>

  <br>

  >
  > <br>
  >
  > In your preferred terminal interface and while logged in as **root**, type the command `pkg update`.
  >
  > If prompted, type `y` as many times as the process asks you to in order to:
  > - Accept to download/update new packages.
  > - Maybe remove old packages and redundancies from the system (depends on what packages are being currently installed in your system).
  > - Accept new space allocation/deallocation depending on the process.
  >
  > When everything is done, your pkg index should be up to date.
  >
  > If you see something like this:
  >
  > ![](https://metin2.download/picture/kVTDN0x72xR9bA76Bwh27oOnEL2CI51N/.png)
  >
  > then your system is already up to date and there is nothing else for pkg to do.
  >
  > <br>
  >
  > Now you can install packages with **pkg** like this (using Git as an example):
  >
  > ```
  > pkg install git
  > ```
  >
  > or
  >
  > ```
  > pkg install devel/git
  > ```
  >
  > **pkg** can find and install packages by name only, but it is also compatible with the port system path like `devel/git` (reference to the path `/usr/ports/devel/git`).
  >
  > <br>
  >
  > ### ✅ You have successfully installed your first package with pkg!
  >
  > <br>
  >
</details>

<br>

<details>
  <summary>
    How to setup ports and use it to install third-party software
  </summary>

  <br>

  >
  > <br>
  >
  > Ports have undergone major changes as new FreeBSD versions became available, select the method for your currently installed version.
  >
  > <br>
  >
  > ### For versions older than 14.0-RELEASE:
  >
  > Use these commands to update your ports tree, hitting `y` to accept the changes every time:
  > ```
  > portsnap fetch extract
  > portsnap fetch update
  > ```
  >
  > <br>
  >
  > ### For versions between 14.0-RELEASE and 14.3-RELEASE that **enabled the ports option during OS installation**:
  >
  > Use this command to fetch the ports tree from the official Github repo and install it in `/usr/ports`:
  > ```
  > git clone --depth 1 https://git.FreeBSD.org/ports.git /usr/ports
  > ```
  >
  > <br>
  >
  > ### For FreeBSD 15.0-RELEASE and versions between 14.0-RELEASE and 14.3-RELEASE that **DID NOT enable the ports option during OS installation**:
  >
  > In order to fetch the latest ports tree, you will need to install Git. Since you don't have ports yet, install it with **pkg** with this command:
  >
  > ```
  > pkg install git
  > ```
  >
  > If you did not run the **pkg** commands from the previous section, the system will ask you some additional questions before installing **Git**. In every case, type `y` and hit ENTER to accept.
  >
  > <br>
  >
  > Once Git downloads and installs with **pkg**, use this command to fetch the ports tree from the official Github repo and install it in `/usr/ports`:
  > ```
  > git clone --depth 1 https://git.FreeBSD.org/ports.git /usr/ports
  > ```
  >
  > **To update your ports tree, simply `cd` into `/usr/ports` and execute `git pull`**:
  > ```
  > cd /usr/ports
  > git pull
  > ```
  >
  > <br>
  >
  > ### How to download and build a port
  >
  > Ports are located in `/usr/ports` within the system, it's where their source code will be downloaded once you run the install command.
  >
  > Note that unlike **pkg**, **ports** cannot install a component by name, you will have to `cd` into 
  >
  > Using **git (devel/git)** as an example, here are the basic commands for **ports** (and a few related ones):
  >
  > - **whereis git**: Search the system for any directory/file references of **git**. If ports are installed correctly, one of the results would be **/usr/ports**/devel/git (the one starting with `/usr/ports` indicates the full path). An empty result indicates that the port does not exist or ports have not been installed correctly.
  > - **make config**: Open up the blue screen with the build options for the selected port (selected port = the one you have `cd`'ed into)
  > - **make config-recursive**: Run the build options for the selected port and all its dependency ports recursevely.
  > - **make install clean**: Build and install the port and its dependencies.
  > - **make clean**: Clean up the port directory from build files.
  > - **make deinstall clean**: Uninstall a port and clean up after.
  >
  > <br>
  >
  > #### So to install Git for example:
  >```
  > cd /usr/ports/devel/git
  > make config-recursive
  > <accept the defaults by keep pressing ENTER>
  > make install clean
  >```
  >
  > <br>
  >
  > ### ✅ You can now find and build ports!
  >
  > <br>
  >
</details>

<br>

**VERY IMPORTANT**: It is highly recommended to AVOID mixing pkg and port installations because of dependency conflicts probability. Stick with one or the other as much as you can for smooth component installations.

<br>

<details>
  <summary>
    List of packages/ports to be installed
  </summary>

  <br>

  >
  > <br>
  >
  > #### Required, for all types of OS installations:
  >
  > - **lang/python**: Can execute python scripts. Required to compile quests, install, start and stop the server.
  > - **devel/cmake**: Required to build the Server Source project.
  > - **devel/dbg**: Required to debug core crashes.
  > - **databases/mariadb118-server**: Required to run a database. You can replace `118` with your preferred version. Automatically installs `mariadb118-client` along with it.
  >
  > <br>
  >
  > #### Required, only for FreeBSD 14.3-RELEASE or lower and FreeBSD 15.0-RELEASE that did **NOT** selected the **devel** option during the OS installation:
  >
  > - **devel/llvm**: Contains `clang`, which is required to build the Server Source. You can also pick `llvm-devel` or `llvm-XX` where `XX` is your preferred version.
  >
  > <br>
  >
  > #### Optional, in case you would like to be able to build other server sources as well (all FreeBSD versions):
  >
  > - **devel/gmake**: Replaces `cmake`, used by most server sources out there.
  > - **devel/makedepend**: Required to work with Makefiles within the source.
  >
  > <br>
  >
  > #### Alternative to `mariadb118-server` in case you are gonna be running other server sources (**CANNOT CO-EXIST WITH MARIADB, CHOOSE ONE OR THE OTHER!**):
  >
  > - **databases/mysql80-server**: Required to run a database. You can replace `80` with your preferred version. Automatically installs `mysql80-client` along with it.
  >
  > **NOTE**: This guide will not be showing how to **configure** MySQL, only MariaDB.
  >
  > <br>
  >
  > #### Optional, for system management (all FreeBSD versions):
  >
  > - **ports-mgmt/portmaster**: Useful only if you are building with ports. Can help manage and update your ports tree and each port individually.
  > - **sysutils/tmux**: Session manager. If working with multiple people in the same VPS instance, you can create and rejoin sessions. Also useful for providers with connection errors that may constantly terminate your PuTTY session, reattaching your tmux session when relogging will display the process/build you had when kicked so you pick it up from where you left it. You can find the `tmux` cheatsheet [here](https://tmuxcheatsheet.com/)
  >
  > <br>
  >
  > ### ✅ You have successfully installed all the required components in order to build the source and run the server!
  >
  > <br>
  >
</details>

<br>

### 📊 Configuring the Database

In order to be able to use the database, we need to set it up first, and then of course access it.

First things first, we need to make the service auto-enable at boot.

<details>
  <summary>
    Here's how:
  </summary>

  <br>

  >
  > <br>
  >
  > In your terminal, type this command:
  >
  > ```
  > echo "mysql_enable=YES" >> /etc/rc.conf
  > ```
  >
  > or edit manually by finding the file with WinSCP and double clicking it to open it in the editor.
  >
  > <br>
  >
</details>

<br>

Next, let's start the service.

<details>
  <summary>
    Here is how to manage a FreeBSD service like MariaDB
  </summary>

  <br>

  >
  > <br>
  >
  > - **service mysql-server status**: Check whether the process is running.
  > - **service mysql-server start**: Start the service.
  > - **service mysql-server stop**: Stop the service.
  > - **service mysql-server restart**: Stop the service and immediately start it again.
  >
  > <br>
  >
  > In order to continue, you now need to start the service.
  >
  > <br>
  >
</details>

<br>

Now let's actually configure the database.

<details>
  <summary>
    Setup
  </summary>

  <br>

  >
  > <br>
  >
  > In your terminal, run this command:
  >
  > ```
  > mariadb-secure-installation
  > ```
  >
  > If you installed **MySQL** instead of **MariaDB**, you need to type this command:
  >
  > ```
  > mysql_secure_installation
  > ```
  >
  > Note: the second command works for both.
  >
  > <br>
  >
  > Next, you will be asked to enter the **root** password. Since this is your first time setting up the database in your instance, there is no password, so hit ENTER.
  >
  > ![](https://metin2.download/picture/frEnBWymAolmQVim7gpeFd4ymhxKbVer/.png)
  >
  > **Note**: MySQL/MariaDB **root** is a different user than your OS's **root** user and their passwords can be different.
  >
  > Now, the installer will ask you whether you want to enable unix socket authentication. Type `n` here as enabling it may give you issues when trying to connect through Navicat/MySQL Workbench.
  >
  > ![](https://metin2.download/picture/9udduxxQUthx8t2IX6Bb8ofFGtkz11T8/.png)
  >
  > Next, you will be asked whether you would like to change the root password. It is strongly recommended to select `y` and hit ENTER.
  >
  > After that, type your desired password for root and press ENTER.
  >
  > Then once again for confirmation and press ENTER.
  >
  > **Note**: Characters won't show but you will be typing.
  >
  > ![](https://metin2.download/picture/lHY7Ucl0EMk1990wkKd7v6L087gung9K/.png)
  >
  > Now the installer will ask whether to remove anonymous users, select `y` here too.
  >
  > ![](https://metin2.download/picture/91Y9bwIj46ik760e5k77449b443Nji17/.png)
  >
  > Next, it will ask you whether to prevent the **root** user from logging in remotely. **Select `n` here**, otherwise you will not be able to login as **root** from Navicat/Workbench.
  >
  > ![](https://metin2.download/picture/2m084gefj2t7I7rKV8TXF2wmJ73XnGP4/.png)
  >
  > Almost there, now it will ask whether to remove the test database, select `y` for this one.
  >
  > ![](https://metin2.download/picture/yqZI4X6eK0LzzEpCzFQfxclvz8xH3Z71/.png)
  >
  > Finally, it will ask whether to reload privileges, this step must be always executed at the end of a change within the database, so select `y`.
  >
  > ![](https://metin2.download/picture/1W642nV6QbnSjApdkIdze7i6Fn8daNly/.png)
  >
  > The installer will now exit, please **restart the `mysql-server` service** as shown in the previous section for the changes to take effect.
  >
  > <br>
  >
  > Now every time you need to login to your database from your terminal, you can simply execute this command:
  >
  > ```
  > mysql -u root -p
  > ```
  >
  > and type in your MySQL/MariaDB root password.
  >
  >In newer versions, localhost connections (from the same IP as the database server) can login with a wrong password or even by pressing ENTER. This is the default behavior in the latest releases.
  >
  > ![](https://metin2.download/picture/L7cKzgME69sFp42JmF2v1S6DO4b10eDD/.png)
  >
  > And if you want to exit the MySQL Client mode, simply execute `exit;` in the terminal. 
  >
  > <br>
  >
</details>

<br>

**A very important final touch!**

Find and edit the `my.cnf` file (either with `ee` or through WinSCP).

<details>
  <summary>
    Here's what to do
  </summary>

  <br>

  >
  > <br>
  >
  > You can find the file in `/usr/local/etc/mysql/my.cnf`. In older versions, it may be in `/usr/local/etc/my.cnf`.
  >
  > ![](https://metin2.download/picture/S6I1b6Htdwu5pCiEEak1QaU32BD2xr0p/.png)
  >
  > Inside the file, you need to add 2 new options under the `[mysqld]` section:
  > - **BIND-ADDRESS**: Set this to `0.0.0.0` or `*` (same thing).
  > - **SQL_MODE**: Set this to `NO_ENGINE_SUBSTITUTION`.
  >
  > ![](https://metin2.download/picture/4OaqNou0Rne130Es0TD9KRr2c2b13zXM/.png)
  >
  > **These settings are required to be able to run the project without errors!**
  >
  > Restart the service for the changes to take effect.
  >
  > <br>
  >
  > Now there's one more thing you need to do in order to be able to login as **root** remotely:
  >
  > Access your database from your terminal by executing:
  >
  > ```
  > mysql -u root -p
  > ```
  >
  > and entering your password.
  >
  > Next, type the following commands to create the `'root'@'%'` and assign full privileges to the new user:
  >
  > ```
  > CREATE USER 'root'@'%' IDENTIFIED BY '<root password here>';
  > GRANT ALL PRIVILEGES ON . TO 'root'@'%' WITH GRANT OPTION;
  > ```
  >
  > What's the difference from the existing **root**?
  >
  > The existing **root** has `localhost` as its host (`'root'@'localhost'`). This means that **root** can only login from the same IP as the database is installed.
  >
  > By creating a **root** user in the `%` host, you are allowing root logins from anywhere, which is what you need to login from Navicat.
  >
  > Next, you need to create the "**serverfiles user**", a user controlled by the serverfiles to read and make changes to the database. This user is recommended to be `localhost` only, especially in production environments.
  >
  > This user also must have full privileges over the database.
  >
  > In the Server project, an automated script has been added for this user, so you may skip this step, just make sure you run the script as shown in the second project.
  >
  > If you wish to add the user now, here is what you need to know:
  >
  > The user's username and password are being defined in the configuration files of the server.
  >
  > **It is CRUCIAL that the username and password within the database match the defined values in the config files of the server!**
  >
  > ![](https://metin2.download/picture/lyeuVZSxihKnSuCHKS7Dq5a7eM5nBEuV/.png)
  >
  > Screenshot from Server's `share/conf/game.txt`.
  >
  > If you select different username/password than what the config files have, **YOU MUST UPDATE THE FILES TO MATCH!** More about this in the second project (Server).
  >
  > To create the user
  >
  > ```
  > CREATE USER 'mt2'@'localhost' IDENTIFIED BY 'mt2@pw';
  > GRANT ALL PRIVILEGES ON . TO 'mt2'@'localhost' WITH GRANT OPTION;
  > ```
  >
  > Whether you added just the remote **root** user, or both **root** and **the serverfiles user**, you must execute this final command before exiting:
  >
  > ```
  > FLUSH PRIVILEGES;
  > exit;
  > ```
  >
  > <br>
  >
  > Once more, **restart the service**.
  >
  > <br>
  >
  > ### ✅ Your database is now fully configured!
  >
  > <br>
  >
  > **Importing the game databases and tables will be shown in the Server part!** Finish this part, and continue in setting up the Server project after you're done.
  >
  > <br>
  >
</details>

<br>

### 🛜 Connecting to the database using Navicat

If everything has been setup correctly, here is how to establish connection to your VM's database through Navicat:

<details>
  <summary>
    Instructions
  </summary>

  <br>

  >
  > <br>
  >
  > Fire up Navicat and click on **Connection** at the top left corner.
  >
  > In the new dialog, select **MariaDB** and then click **Next**.
  >
  > ![](https://metin2.download/picture/2Wr35nHtx4jTfKkmlw6DFvRPKQIyilsf/.png)
  >
  > <br>
  >
  > Now in this next step:
  >
  > 1. **Connection Name**: Select a meaningful name for your connection. This is what you'll be seeing at the initial screen's left-side panel.
  > 2. **Host**: Your IP address from the output of the `ifconfig` command.
  > 3. **Port**: Leave that as **3306**.
  > 4. **User Name**: Here type **root** (obviously!)
  > 5. **Password**: Your **root** password here.
  > 6. **Save passsword**: Optional, not recommended for production environments or devices you share with others.
  > 7. **Finally**, you can test whether Navicat successfully connects to your database.
  >
  > ![](https://metin2.download/picture/Xs2yzAmyIuAy7mU7QrFu2Q6Fm4vEGida/.png)
  >
  > <br>
  >
  > If everything is setup correctly, you should be seeing this test result:
  >
  > ![](https://metin2.download/picture/ngGm0pv12rgwhTkA8n6yN8bO0ns6cyL0/.png)
  >
  > Click **OK** to save this connection to your left panel.
  >
  > In order to connect to your database, you can double click on your saved instance from the panel at the left.
  >
  > <br>
  >
  > ### ✅ You can now connect successfully to your database remotely using database managemnt software!
  >
  > <br>
  >
</details>

<br>

### ⬇️ Obtaining the Server Source

The moment is here! In your terminal, `cd` into your desired location or create a new folder wherever you want and download the project using `git`.

<details>
  <summary>
    Here's how
  </summary>

  <br>

  >
  > <br>
  >
  >
  > This example will use `/usr/home/src` as the working directory.
  >
  > Execute these commands:
  >
  > ```
  > mkdir /usr/home/src
  > cd /usr/home/src
  > git clone https://github.com/d1str4ught/m2dev-server-src.git .
  > ```
  > Mind the `.` in the end of the last command.
  >
  > With the `.`, the project will be cloned right in `/usr/home/src`, while without it, it will be cloned as `/usr/home/src/m2dev-server-src`.
  >
  > This is just a preference, either way is fine.
  >
  > <br>
  >
  > ### ✅ You have successfully obtained the Server Source project!
  >
  > <br>
  >
</details>

<br>

### 🛠️ Building the Source Code

Building the project is extremely simple, if all packages/ports are being installed correctly.

<details>
  <summary>
    Instructions
  </summary>

  <br>

  >
  > <br>
  >
  > In your terminal, `cd` in your project's root working directory:
  >
  > ```
  > cd /usr/home/src
  > ```
  >
  > Now initialize the build:
  >
  > ```
  > cmake -S . -B build
  > ```
  >
  > A new `build` folder has been created in your project's root directory. This folder contains all the build files and configurations.
  >
  > Now, initiate the build!
  >
  > ```
  > cmake --build build
  > ```
  >
  > This process will be slow. If you wanna accelerate this process use this command:
  >
  > ```
  > cmake --build build -j$(( $(sysctl -n hw.ncpu) + 2 ))
  > ```
  >
  > What this does is enforcing multiple jobs at the same time using the available **CPU Cores** that were being set at the beginning, in the VM's creation process.
  >
  > This is where the power of multiple CPU cores comes in. If you set a high amount of cores available to your VM, the build should only take a couple of minutes (CPU strength also matters for this speedup).
  >
  > <br>
  > In the end, the finished build process should look like this:
  >
  > ![](https://metin2.download/picture/c4k0YIFtLbb7j057XluMUlyp1j6eQZe6/.png)
  >
  > <br>
  >
  > Now you need to distribute the built binaries in their respective locations within the **Server** project and assign full system permissions (`0777`).
  >
  > To do this, you need to have the Server project cloned in your VM the same way you cloned this project.
  >
  > If you **DON'T** have the Server project cloned yet, skip this step, we will revisit it in the Server project anyway.
  >
  > If you **DO** have the Server project available in your VM, execute these commands (**replace the directories with your own structure!!!**):
  >
  > ```
  > cd /usr/home/src/build/bin
  >
  > cp ./game /usr/home/game/share/bin/game
  > chmod 777 /usr/home/game/share/bin/game
  >
  > cp ./db /usr/home/game/share/bin/db
  > chmod 777 /usr/home/game/share/bin/db
  >
  > cp ./qc /usr/home/game/share/locale/english/quest/qc
  > chmod 777 /usr/home/game/share/locale/english/quest/qc
  > ```
  > **Note**: For `qc`, replace `english` with your locale if you have changed it.
  >
  > <br>
  >
  > ### ✅ You have successfully built the Server Source!
  >
  > <br>
  >
</details>

<br>
<br>

---

<br>
<br>

## ![Windows](https://metin2.download/picture/kHdjS3dGuooT62j9qmhNCJyZhP69Vq89/.png) **Windows**

This process will be relatively way easier than the **FreeBSD** method because it skips the installation of a whole new OS from scratch.

### 🧱 Software Prerequisites

<details>
  <summary>
    Please make sure that you have installed the following software in your machine before continuing:
  </summary>

  <br>

  > <br>
  >
  >  - ![Visual Studio](https://metin2.download/picture/B6U1Pg0lMlA486D1ekVLIytP72pDP8Yg/.png)&nbsp;&nbsp;**Visual Studio**:&nbsp;&nbsp;The software used to edit and compile the source code. [Download](https://visualstudio.microsoft.com/vs/)
  >
  >  - ![Visual Studio Code](https://metin2.download/picture/Hp33762v422mjz6lgil91Gey380NwA7j/.png)&nbsp;&nbsp;**Visual Studio Code (VS Code)**:&nbsp;&nbsp;A lighter alternative to Visual Studio, harder to build the project in this software but it is recommended for code editing. [Download](https://code.visualstudio.com/Download)
  > - ![MariaDB](https://metin2.download/picture/1ql7r3eg1h52EeTz68icMgUEGHmdA3ez/.jpg)&nbsp;&nbsp;**MariaDB**:&nbsp;&nbsp;You can also use the official **MySQL/MariaDB Workbench clients from Oracle**, which contains the server and the client in one installer, but this guide will only show how to download the server and access it through Navicat. [Download](https://mariadb.com/downloads/)
  >  - ![Navicat Premium Lite](https://metin2.download/picture/2dAmE5sX2U3VtErqhijVB3hRI57Z7yWp/.png)&nbsp;&nbsp;**Navicat Premium Lite (Free Version)**:&nbsp;&nbsp;You can also use the official **MySQL/MariaDB Workbench clients from Oracle**, or any other database management software you prefer, but most people in this space are more familiar with Navicat. [Download](https://www.navicat.com/en/download/navicat-premium-lite)
  > - ![Git](https://metin2.download/picture/eCpg436LhgG1zg9ZqvyfcANS68F60y6O/.png)&nbsp;&nbsp;**Git**:&nbsp;&nbsp;Used to clone the repositories in your Windows machine. [Download](https://git-scm.com/install/windows)
  > - ![CMake](https://metin2.download/picture/6O8Ho9N0XScDaLLL8h0lrkh8DcKlgJ6M/.png)&nbsp;&nbsp;**CMake**:&nbsp;&nbsp;Required for setting up and configuring the build of the source code. [Download](https://git-scm.com/install/windows)
  > - ![Notepad++](https://metin2.download/picture/7Vf5Yv1T48nHprT2hiH0VfZx635HAZP2/.png)&nbsp;&nbsp;**Notepad++ (optional but recommended)**:&nbsp;&nbsp;Helps with quick, last minute edits. [Download](https://notepad-plus-plus.org/downloads/)
  >  - ![Python](https://metin2.download/picture/PQiBu5Na1ld90rixm0tVgstMxR43OpIn/.png)&nbsp;&nbsp;**Python**:&nbsp;&nbsp;The software used to execute python scripts. It is **recommended to ADD TO PATH** at the end of the installation. [Download](https://www.python.org/downloads/)
  >
  > <br>
  >

</details>

<br>

### 👁️ Required Visual Studio packages

Make sure you have installed these packages with Visual Studio Installer in order to compile C++ codebases:

<details>
  <summary>
    Packages
  </summary>

  <br>

  >
  > <br>
  >
  > ![](https://metin2.download/picture/Rh6PJwXf0J6Y3TZFg7Zx8HVBUGNXDVDN/.png)
  >
  > ![](https://metin2.download/picture/1eoYD0IJB5k9c9BhhyWTjDVm6zidv8rM/.png)
  >
  > **Note**: **Windows 11 SDK**'s can be replaced by **Windows 10 SDK**'s, but it is recommended to install one of them.
  >
  > <br>
  >
</details>

<br>

### 📊 Installing MariaDB Server on Windows

It should be easy, but here are some notes about this installation:

<details>
  <summary>
    Installation process
  </summary>

  <br>

  >
  > <br>
  >
  > Execute the installer and click **Next** (accept the Terms as well), until you see this screen:
  >
  > ![](https://metin2.download/picture/w6SWzs19oj3FSlTQR7A0km0xnhtuytk9/.png)
  >
  > Here set a **root** password and choose whether you want to enable **remote root access** (**root** logins from different IPs than your device's), you should be totally fine and safer without this option enabled, then click **Next**.
  >
  > In the next screen, accept the defaults and hit **Next**, then begin the installation.
  >
  > <br>
  >
  > You should now be seeing the **MariaDB** service in your Services window:
  >
  > ![](https://metin2.download/picture/T65UcOhc5M38FLwUGIF7OQ7HdQwlLYtK/.png)
  >
  > **If you ever find yourself unable to connect to your database with your terminal or Navicat, check whether this service is running or not.**
  >
  > <br>
  >
  > ### ✅ You have successfully installed MariaDB Server to your Windows machine!
  >
  > <br>
  >
</details>

<br>

### ⚙️ Configuring MariaDB Server for running the serverfiles

The following adjustments are **required** for a smooth and issue-free experience.

<details>
  <summary>
    Let's get started
  </summary>

  <br>

  >
  > <br>
  >
  > Find the file **my.ini** and open it with Notepad++ or any text editor of your choice.
  >
  > The file should be in the **data** folder, inside your installation directory.
  >
  > The default installation directory for MariaDB 12.1 would be `C:\Program Files\MariaDB 12.1\data\my.ini`
  >
  > Inside this file, in the `[mysqld]` section, add the following lines:
  >
  > ```
  > bind-address=0.0.0.0
  > sql_mode=NO_ENGINE_SUBSTITUTION
  > ```
  >
  > It should be looking like this:
  >
  > ![](https://metin2.download/picture/eK6u6THwupjRB0CdbtUEC60rZnjsidId/.png)
  >
  > When you're done, restart your service from the Services window:
  >
  > ![](https://metin2.download/picture/K3NF5JkHk2f3eKXxf48SeWTHfDNFs2lN/.png)
  >
  > <br>
  >
  > Next, you need to create the "**serverfiles user**", a user controlled by the serverfiles to read and make changes to the database. This user is recommended to be `localhost` only, especially in production environments.
  >
  > This user also must have full privileges over the database.
  >
  > In the Server project, an automated script has been added for this user, so you may skip this step, just make sure you run the script as shown in the second project.
  >
  > If you wish to add the user now, here is what you need to know:
  >
  > The user's username and password are being defined in the configuration files of the server.
  >
  > **It is CRUCIAL that the username and password within the database match the defined values in the config files of the server!**
  >
  > ![](https://metin2.download/picture/lyeuVZSxihKnSuCHKS7Dq5a7eM5nBEuV/.png)
  >
  > Screenshot from Server's `share/conf/game.txt`.
  >
  > If you select different username/password than what the config files have, **YOU MUST UPDATE THE FILES TO MATCH!** More about this in the second project (Server).
  >
  > To create the user, open up a terminal inside your **MariaDB installation directory, in the bin folder** (or `cd` in that folder) and type:
  >
  > ```
  > .\mariadb.exe -u root -p
  > ```
  >
  > ![](https://metin2.download/picture/9RaavRzJ1Nv4kfL9213Q48xdWs6vzQBF/.png)
  >
  > Enter your password and hit ENTER. Then execute these commands:
  >
  > ```
  > CREATE USER 'mt2'@'localhost' IDENTIFIED BY 'mt2@pw';
  > GRANT ALL PRIVILEGES ON *.* TO 'mt2'@'localhost' WITH GRANT OPTION;
  > FLUSH PRIVILEGES;
  > exit;
  > ```
  >
  > Once again, restart the service.
  >
  > <br>
  >
  > ### ✅ You have successfully configured MariaDB Server to run the project!
  >
  > <br>
  >
</details>

<br>

### 🛜 Connecting to the database using Navicat

If everything has been setup correctly, here is how to establish connection to your localhost's database through Navicat:

<details>
  <summary>
    Instructions
  </summary>

  <br>

  >
  > <br>
  >
  > Fire up Navicat and click on **Connection** at the top left corner.
  >
  > In the new dialog, select **MariaDB** and then click **Next**.
  >
  > ![](https://metin2.download/picture/2Wr35nHtx4jTfKkmlw6DFvRPKQIyilsf/.png)
  >
  > <br>
  >
  > Now in this next step:
  >
  > 1. **Connection Name**: Select a meaningful name for your connection. This is what you'll be seeing at the initial screen's left-side panel.
  > 2. **Host**: Your machine's IP address, the word `localhost`, or `127.0.0.1`, any one of them will get you connected.
  > 3. **Port**: Leave that as **3306**.
  > 4. **User Name**: Here type **root** (obviously!)
  > 5. **Password**: Your **root** password here.
  > 6. **Save passsword**: Optional, not recommended for production environments or devices you share with others.
  > 7. **Finally**, you can test whether Navicat successfully connects to your database.
  >
  > ![](https://metin2.download/picture/68KKiK8eLQ214WHfj033ZV23t0lAzE8S/.png)
  >
  > <br>
  >
  > If everything is setup correctly, you should be seeing this test result:
  >
  > ![](https://metin2.download/picture/xR4LVh0cgyznE5Z50uJmDd0kNfntMxst/.png)
  >
  > Click **OK** to save this connection to your left panel.
  >
  > In order to connect to your database, you can double click on your saved instance from the panel at the left.
  >
  > <br>
  >
  > ### ✅ You can now connect successfully to your database using database managemnt software!
  >
  > <br>
  >
</details>

<br>


### ⬇️ Obtaining the Server Source

The moment is here! In your terminal, `cd` into your desired location or create a new folder wherever you want and download the project using `Git`.

<details>
  <summary>
    Here's how
  </summary>

  <br>

  >
  > <br>
  >
  >
  > Open up your terminal inside or `cd` into your desired folder and type this command:
  >
  > ```
  > git clone https://github.com/d1str4ught/m2dev-server-src.git
  > ```
  >
  > <br>
  >
  > ### ✅ You have successfully obtained the Server Source project!
  >
  > <br>
  >
</details>

<br>

### 🛠️ Building the Source Code

Building the project is extremely simple, if all Visual Studio components are being installed correctly.

<details>
  <summary>
    Instructions
  </summary>

  <br>

  >
  > <br>
  >
  > Open up your terminal inside, or `cd` in your project's root working directory and initialize the build with this command:
  >
  > ```
  > cmake -S . -B build
  > ```
  >
  > A new `build` folder has been created in your project's root directory. This folder contains all the build files and configurations, along with the `sln` file to open the project in Visual Studio.
  >
  > ![](https://metin2.download/picture/Ezrm2cQ60282BQw0z1WmrxGdv0PIQDG7/.png)
  >
  > ![](https://metin2.download/picture/JCfv3N4z9WEhewjxFYTYTZjt9n18RVO4/.png)
  >
  > Double click on that file to launch Visual Studio and load the project.
  >
  > In the Solution Explorer, select all the projects minus the container folders, right click on one of the selected items, and click **Properties**
  >
  > ![](https://metin2.download/picture/rJZUX58qr98eHjOlha9p4l8NG8Eg8lEc/.png)
  >
  > Next, make sure that the following settings are adjusted like this:
  >
  > 1. **Windows SDK Version** should be the latest of Windows 10. It is not recommended to select any Windows 11 versions yet if avalable.
  > 2. **Platform Toolset** is the most important part for your build to succeed! Select the highest number you see. **v145** is for Visual Studio 2026. If you are running Visual Studio 2022 you won't have that, you will have **v143**, select that one, same goes for older Visual Studio versions.
  > 3. **C++ Language Standard** should be C++20 as it is the new standard defined in the CMakeList.txt files as well. Might as well set it like that for all dependencies.
  > 4. **C Language Standard** should be C17 as it is the new standard defined in the CMakeList.txt files as well. Might as well set it like that for all dependencies.
  >
  > Once done, click Apply and then OK to close this dialog.
  >
  > ![](https://metin2.download/picture/2uacNZ4zOCYfI6vmigMTFwbuQ4tcmZ1C/.png)
  >
  > After that, in the toolbar at the top of the window, select your desired output configuration:
  >
  > ![](https://metin2.download/picture/CGVGy090Z4RhCN9922ZUPztAj0j7yQA2/.png)
  >
  > Finally, click on the **Build** option at the top and select **Build Solution**, or simply press **CTRL+SHIFT+B** in your keyboard with all the projects selected.
  >
  > ![](https://metin2.download/picture/FF24Sb1b4CWv2u0thji7ChZiQR8W3WFn/.png)
  >
  > **Note**: if this is **NOT** your first build after executing the `cmake -S . -B build` command for this workspace, it is recommended to click **Clean Solution** before **Build Solution**.
  >
  > <br>
  >
  > Where to find your compiled binaries:
  >
  > Inside the **build** folder in your cloned repository, you should have a **bin** folder and inside that, you should have a **Debug**, **Release**, **RelWithDebInfo** or **MinSizeRel** folder, depending on your build configuration selection.
  >
  > In that folder you should be seeing all your binaries:
  >
  > ![](https://metin2.download/picture/WVd2G1sCMzgl2caS60yM87j7rQI35eQs/.png)
  >
  > If you did **NOT** install the **Server** project yet, you are done here.
  >
  > If you **HAVE** the **Server** project installed, paste the 3 `.exe` files in these locations inside the Server project:
  >
  > - **game.exe**: inside `share\bin\game.exe`
  > - **db.exe**: inside `share\bin\db.exe`
  > - **qc.exe**: inside `share\locale\english\quest\qc.exe` (replace `english` with your locale if you have changed it)
  >
  > <br>
  >
  > ### ✅ You have successfully built the Server Source!
  >
  > <br>
  >
</details>

<br>
<br>

---

<br>
<br>

## Recap
After finishing this part, you should now have knowledge of:

 - Creating a virtual machine and connecting it to the internet
 - Installing the FreeBSD operating system in a VM from scratch
 - Connecting to your FreeBSD VM with PuTTY and WinSCP
 - Enabling remote root access in FreeBSD
 - Installing FreeBSD components either with pkg or with ports
 - Managing FreeBSD services
 - Assigning file permissions in FreeBSD
 - Installing Visual Studio and its components for C++ builds
 - Installing and configuring MariaDB from scratch in Windows/FreeBSD, adding and editing users, assigning permissions and accessing your database through Navicat
 - Cloning Github repositories
 - Configuring builds with CMake
 - Building with FreeBSD or/and Visual Studio

<br>

## 🔥 The Server Source part of the guide is complete!

<br>

## Next steps
After following either the **FreeBSD** method or the **Windows** method, you should be ready to proceed to the installation and configuration of the [Server project](https://github.com/d1str4ught/m2dev-server)

⭐ **NEW**: We are now on Discord, feel free to [check us out](https://discord.gg/ETnBChu2Ca)!