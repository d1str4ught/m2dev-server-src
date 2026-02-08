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

---

### Networking Modernization Roadmap

A 5-phase modernization of the entire client/server networking stack — packet format, buffer management, handshake protocol, connection architecture, and packet dispatching. Every phase is complete and verified on both client and server.

---

#### Phase 1 — Packet Format + Buffer System + Memory Safety

Replaced the legacy 1-byte packet headers and raw C-style buffers with a modern, uniform protocol.

##### What changed
* **2-byte headers + 2-byte length prefix** — All packet types (`CG::`, `GC::`, `GG::`, `GD::`, `DG::`) now use `uint16_t` header + `uint16_t` length. This increases the addressable packet space from 256 to 65,535 unique packet types and enables safe variable-length parsing
* **Namespaced packet headers** — All headers moved from flat `HEADER_CG_*` defines to C++ namespaces: `CG::MOVE`, `GC::PING`, `GG::LOGIN`, `GD::PLAYER_SAVE`, `DG::BOOT`. Subheaders similarly namespaced: `GuildSub::GC::LOGIN`, `ShopSub::CG::BUY`, etc.
* **RAII RingBuffer** — All raw `buffer_t` / `LPBUF` / `new[]`/`delete[]` patterns replaced with a single `RingBuffer` class featuring lazy compaction at 50% read position, exponential growth, and inlined accessors
* **PacketReader / PacketWriter** — Type-safe helpers that wrap buffer access with bounds checking, eliminating raw pointer arithmetic throughout the codebase
* **Sequence system modernized** — Packet sequence tracking retained for debugging but fixed to byte offset 4 (after header + length)
* **SecureCipher** — XChaCha20-Poly1305 stream cipher for all post-handshake traffic (see Encryption section above)

##### What was removed
* `buffer.h` / `buffer.cpp` (legacy C buffer library)
* All `LPBUF`, `buffer_new()`, `buffer_delete()`, `buffer_read()`, `buffer_write()` calls
* Raw `new[]`/`delete[]` buffer allocations in DESC classes
* 1-byte header constants (`HEADER_CG_*`, `HEADER_GC_*`, etc.)

##### Why
The legacy 1-byte header system limited the protocol to 256 packet types (already exhausted), raw C buffers had no bounds checking and were prone to buffer overflows, and the flat namespace caused header collisions between subsystems.

---

#### Phase 2 — Modern Buffer System *(merged into Phase 1)*

All connection types now use `RingBuffer` uniformly.

##### What changed
* **All DESC types** (`DESC`, `CLIENT_DESC`, `DESC_P2P`) use `RingBuffer` for `m_inputBuffer`, `m_outputBuffer`, and `m_bufferedOutputBuffer`
* **PeerBase** (db layer) ported to `RingBuffer`
* **TEMP_BUFFER** (local utility for building packets) backed by `RingBuffer`

##### What was removed
* `libthecore/buffer.h` and `libthecore/buffer.cpp` — the entire legacy buffer library

##### Why
The legacy buffer system used separate implementations across different connection types, had no RAII semantics (manual malloc/free), and offered no protection against buffer overflows.

---

#### Phase 3 — Simplified Handshake

Replaced the legacy multi-step handshake (4+ round trips with time synchronization and UDP binding) with a streamlined 1.5 round-trip flow.

##### What changed
* **1.5 round-trip handshake** — Server sends `GC::KEY_CHALLENGE` (with embedded time sync), client responds with `CG::KEY_RESPONSE`, server confirms with `GC::KEY_COMPLETE`. Session is encrypted from that point forward
* **Time sync embedded** — Initial time synchronization folded into `GC::KEY_CHALLENGE`; periodic time sync handled by `GC::PING` / `CG::PONG`
* **Handshake timeout** — 5-second expiry on handshake phase; stale connections are automatically cleaned up in `DestroyClosed()`

##### What was removed
* **6 dead packet types**: `CG_HANDSHAKE`, `GC_HANDSHAKE`, `CG_TIME_SYNC`, `GC_TIME_SYNC`, `GC_HANDSHAKE_OK`, `GC_BINDUDP`
* **Server functions**: `StartHandshake()`, `SendHandshake()`, `HandshakeProcess()`, `CreateHandshake()`, `FindByHandshake()`, `m_map_handshake`
* **Client functions**: `RecvHandshakePacket()`, `RecvHandshakeOKPacket()`, `m_HandshakeData`, `SendHandshakePacket()`
* ~12 server files and ~10 client files modified

##### Why
The original handshake required 4+ round trips, included dead UDP binding steps, had no timeout protection (stale connections could linger indefinitely), and the time sync was a separate multi-step sub-protocol that added latency to every new connection.

---

#### Phase 4 — Unified Connection (Client-Side Deduplication)

Consolidated duplicated connection logic into the base `CNetworkStream` class.

##### What changed
* **Key exchange** (`RecvKeyChallenge` / `RecvKeyComplete`) moved from 4 separate implementations to `CNetworkStream` base class
* **Ping/pong** (`RecvPingPacket` / `SendPongPacket`) moved from 3 separate implementations to `CNetworkStream` base class
* **CPythonNetworkStream** overrides `RecvKeyChallenge` only for time sync, delegates all crypto to base
* **CGuildMarkDownloader/Uploader** — `RecvKeyCompleteAndLogin` wraps base + sends `CG::MARK_LOGIN`
* **CAccountConnector** — Fixed raw `crypto_aead` bug (now uses base class `cipher.DecryptToken`)
* **Control-plane structs** extracted to `EterLib/ControlPackets.h` (Phase, Ping, Pong, KeyChallenge, KeyResponse, KeyComplete)
* **CGuildMarkUploader** — `m_pbySymbolBuf` migrated from `new[]`/`delete[]` to `std::vector<uint8_t>`

##### What was removed
* ~200 lines of duplicated code across `CAccountConnector`, `CGuildMarkDownloader`, `CGuildMarkUploader`, and `CPythonNetworkStream`

##### Why
The same key exchange and ping/pong logic was copy-pasted across 3-4 connection subclasses, leading to inconsistent behavior (the `CAccountConnector` had a raw crypto bug), difficult maintenance, and unnecessary code volume.

---

#### Phase 5 — Packet Handler Registration / Dispatcher

Replaced giant `switch` statements with `std::unordered_map` dispatch tables for O(1) packet routing.

##### What changed

**Client:**
* `CPythonNetworkStream` — Phase-specific handler maps for Game, Loading, Login, Select, and Handshake phases
* Registration pattern: `m_gameHandlers[GC::MOVE] = &CPythonNetworkStream::RecvCharacterMovePacket;`
* Dispatch: `DispatchPacket(m_gameHandlers)` — reads header, looks up handler, calls it

**Server:**
* `CInputMain`, `CInputDead`, `CInputAuth`, `CInputLogin`, `CInputP2P`, `CInputHandshake`, `CInputDB` — all converted to dispatch tables
* `CInputDB` uses 3 template adapters (`DataHandler`, `DescHandler`, `TypedHandler`) + 14 custom adapters for the diverse DB callback signatures

##### What was removed
* All `switch (header)` blocks across 7 server input processors and 5 client phase handlers
* ~3,000 lines of switch/case boilerplate

##### Why
The original dispatch used switch statements with 50-100+ cases each. Adding a new packet required modifying a massive switch block, which was error-prone and caused merge conflicts. The table-driven approach enables O(1) lookup, self-documenting handler registration, and trivial addition of new packet types.

---

### Post-Phase 5 Cleanup

Follow-up tasks after the core roadmap was complete.

#### One-Liner Adapter Reformat
* 24 adapter methods across 4 server files (`input_main.cpp`, `input_p2p.cpp`, `input_auth.cpp`, `input_login.cpp`) reformatted from single-line to multi-line for readability

#### MAIN_CHARACTER Packet Merge
* **4 mutually exclusive packets** (`GC::MAIN_CHARACTER`, `MAIN_CHARACTER2_EMPIRE`, `MAIN_CHARACTER3_BGM`, `MAIN_CHARACTER4_BGM_VOL`) merged into a single unified `GC::MAIN_CHARACTER` packet
* Single struct always includes BGM fields (zero when unused — 29 extra bytes on a one-time-per-load packet)
* 4 nearly identical client handlers merged into 1
* 3 redundant server send paths merged into 1

#### UDP Leftover Removal
* **7 client files deleted**: `NetDatagram.h/.cpp`, `NetDatagramReceiver.h/.cpp`, `NetDatagramSender.h/.cpp`, `PythonNetworkDatagramModule.cpp`
* **8 files edited**: Removed dead stubs (`PushUDPState`, `initudp`, `netSetUDPRecvBufferSize`, `netConnectUDP`), declarations, and Python method table entries
* **Server**: Removed `socket_udp_read()`, `socket_udp_bind()`, `__UDP_BLOCK__` define

#### Subheader Dispatch
* Extended the Phase 5 table-driven pattern to subheader switches with 8+ cases
* **Client**: Guild (19 sub-handlers), Shop (10), Exchange (8) in `PythonNetworkStreamPhaseGame.cpp`
* **Server**: Guild (15 sub-handlers) in `input_main.cpp`
* Small switches intentionally kept as-is: Messenger (5), Fishing (6), Dungeon (2), Server Shop (4)

---

### Performance Audit & Optimization

Comprehensive audit of all Phase 1-5 changes to identify and eliminate performance overhead.

#### Debug Logging Cleanup
* **Removed all hot-path `TraceError`/`Tracef`/`sys_log`** from networking code on both client and server
* Client: `NetStream.cpp`, `SecureCipher.cpp`, `PythonNetworkStream*.cpp` — eliminated per-frame and per-packet traces that caused disk I/O every frame
* Server: `desc.cpp`, `input.cpp`, `input_login.cpp`, `input_auth.cpp`, `SecureCipher.cpp` — eliminated `[SEND]`, `[RECV]`, `[CIPHER]` logs that fired on every packet

#### Packet Processing Throughput
* **`MAX_RECV_COUNT` 4 → 32** — Game phase now processes up to 32 packets per frame (was 4, severely limiting entity spawning on map entry)
* **Loading phase while-loop** — Changed from processing 1 packet per frame to draining all available packets, making phase transitions near-instant

#### Flood Check Optimization
* **Replaced `get_dword_time()` with `thecore_pulse()`** in `DESC::CheckPacketFlood()` — eliminates a `gettimeofday()` syscall on every single packet received. `thecore_pulse()` is cached once per game-loop iteration

#### Flood Protection
* **Per-IP connection limits** — Configurable maximum connections per IP address (`flood_max_connections_per_ip`, default: 10)
* **Global connection limits** — Configurable maximum total connections (`flood_max_global_connections`, default: 8192)
* **Per-second packet rate limiting** — Connections exceeding `flood_max_packets_per_sec` (default: 300) are automatically disconnected
* **Handshake timeout** — 5-second expiry prevents connection slot exhaustion from incomplete handshakes

---

### DDoS / Flood Mitigation

Two-layer defense against UDP floods and TCP SYN floods, integrated directly into the game server process.

#### Layer 1 — UDP Sink Sockets

Dummy UDP sockets bound to each game and P2P port with a minimal receive buffer (`SO_RCVBUF = 1`). The sockets are never read from — once the tiny kernel buffer fills, incoming UDP packets are silently dropped without generating ICMP port-unreachable replies. This prevents the server from being used as an ICMP reflection amplifier and reduces kernel CPU overhead from UDP floods.

* **Zero overhead** — No threads, no polling, no syscalls. The kernel handles everything.
* **Always active** — Created unconditionally at startup alongside the TCP listeners
* **Defense-in-depth** — Acts as a fallback if iptables rules cannot be installed (e.g., non-root)

#### Layer 2 — Kernel-Level Firewall (`FirewallManager`)

The `FirewallManager` singleton programmatically installs firewall rules at server startup, dropping malicious traffic at the kernel level — before it reaches the socket/application layer. This is dramatically more efficient than socket-level drops at high packet rates (700k+ pps).

* **FreeBSD** — Uses `ipfw` numbered rules (deterministic rule base per port)
* **Linux** — Uses `iptables` with a dedicated chain per process (e.g., `M2_GUARD_11011`)
* **Windows** — No-op stubs (compiles but does nothing). Windows is dev-only

##### What it does
* **Drops all unsolicited inbound UDP** — Packets are rejected at the kernel firewall layer with near-zero CPU cost per packet
* **Rate-limits TCP SYN** on game and P2P ports — Default: 500 new connections/sec (per-source limit on FreeBSD, rate limit on Linux). Protects against SYN flood attacks while allowing legitimate mass logins (e.g., 1000 players at server launch)
* **Drops ICMP port-unreachable** — Prevents reflection/amplification attacks
* **Per-process isolation** — Each game process installs its own rules to avoid conflicts in multi-channel deployments
* **Crash recovery** — On startup, stale rules from previous crashes are automatically cleaned up before installing fresh rules
* **Graceful cleanup** — All rules are removed on normal shutdown

##### Configuration (`conf/game.txt`)

| Token | Default | Description |
|-------|---------|-------------|
| `firewall_enable` | `0` | Enable/disable the firewall manager (0=off, 1=on) |
| `firewall_tcp_syn_limit` | `500` | Max new TCP SYN connections/sec per port |
| `firewall_tcp_syn_burst` | `1000` | SYN burst allowance before rate limiting kicks in (Linux only) |

##### FreeBSD setup (ipfw)

Before enabling the firewall manager, the `ipfw` kernel module must be loaded with a default-allow policy. Without the allow rule, loading ipfw will block all traffic and lock you out.

```bash
# Load ipfw module and immediately add a default allow rule
kldload ipfw && /sbin/ipfw -q add 65000 allow ip from any to any
```

To persist across reboots, add to `/boot/loader.conf`:
```
ipfw_load="YES"
```

And to `/etc/rc.conf`:
```
firewall_enable="YES"
firewall_type="open"
```

##### Verification

FreeBSD:
```bash
# Check rules are installed (each process uses rule base 50000 + (port % 1000) * 10)
/sbin/ipfw list | grep 500

# Verify rules are cleaned up after shutdown
/sbin/ipfw list | grep 500  # should show no matching rules
```

Linux:
```bash
# Check rules are installed
iptables -L M2_GUARD_11011 -n -v

# Verify chain is cleaned up after shutdown
iptables -L M2_GUARD_11011  # should fail with "No chain/target/match by that name"
```

##### Requirements
* **Root access** — Required on both FreeBSD and Linux for firewall rule installation. If not root, logs a warning and the server continues without firewall rules (Layer 1 UDP sinks still protect)

---

### Pre-Phase 3 Cleanup

Preparatory cleanup performed before the handshake simplification.

* **File consolidation** — Merged scattered packet definitions into centralized header files
* **Alias removal** — Removed legacy `#define` aliases that mapped old names to new identifiers
* **Monarch system removal** — Completely removed the unused Monarch (emperor) system from both client and server, including all related packets, commands, quest functions, and UI code
* **TrafficProfiler removal** — Removed the `TrafficProfiler` class and all references (unnecessary runtime overhead)
* **Quest management stub removal** — Removed empty `questlua_mgmt.cpp` (monarch-era placeholder with no functions)

---

### Summary of Removed Legacy Systems

A consolidated reference of all legacy systems, files, and dead code removed across the entire modernization effort.

| System | What was removed | Replaced by |
|--------|-----------------|-------------|
| **Legacy C buffer** | `buffer.h`, `buffer.cpp`, all `LPBUF`/`buffer_new()`/`buffer_delete()` calls, raw `new[]`/`delete[]` buffer allocations | RAII `RingBuffer` class |
| **1-byte packet headers** | All `HEADER_CG_*`, `HEADER_GC_*`, `HEADER_GG_*`, `HEADER_GD_*`, `HEADER_DG_*` defines | 2-byte namespaced headers (`CG::`, `GC::`, `GG::`, `GD::`, `DG::`) |
| **Old handshake protocol** | 6 packet types (`CG_HANDSHAKE`, `GC_HANDSHAKE`, `CG_TIME_SYNC`, `GC_TIME_SYNC`, `GC_HANDSHAKE_OK`, `GC_BINDUDP`), all handshake functions and state | 1.5 round-trip key exchange (`KEY_CHALLENGE`/`KEY_RESPONSE`/`KEY_COMPLETE`) |
| **UDP networking** | 7 client files (`NetDatagram*.h/.cpp`, `PythonNetworkDatagramModule.cpp`), server `socket_udp_read()`/`socket_udp_bind()`/`__UDP_BLOCK__` | Removed entirely (game is TCP-only) |
| **Old sequence system** | `m_seq`, `SetSequence()`, `GetSequence()`, old sequence variables | Modernized sequence at fixed byte offset 4 |
| **TrafficProfiler** | `TrafficProfiler` class and all references | Removed entirely |
| **Monarch system** | All monarch/emperor packets, commands (`do_monarch_*`), quest functions (`questlua_monarch.cpp`, `questlua_mgmt.cpp`), UI code, GM commands | Removed entirely (unused feature) |
| **Legacy crypto** | Crypto++, Panama cipher, TEA, DH2, Camellia, XTEA, `adwClientKey[4]`, `LSS_SECURITY_KEY` | libsodium (X25519 + XChaCha20-Poly1305) |
| **Switch-based dispatch** | Giant `switch (header)` blocks (50-100+ cases each) across 7 server input processors and 5 client phase handlers | `std::unordered_map` dispatch tables |
| **Duplicated connection code** | Key exchange and ping/pong copy-pasted across 3-4 client subclasses | Consolidated in `CNetworkStream` base class |
