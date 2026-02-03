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
