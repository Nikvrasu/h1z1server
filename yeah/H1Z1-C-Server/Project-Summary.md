Let me read the current state of the key files before writing the summary:Here is the comprehensive CLAUDE.md. Paste this as the entire contents of your `CLAUDE.md` file:

---

```markdown
# H1Z1-C-Server — Claude Context

## Project Overview
H1Z1: King of the Kill (v0.23.4.161178) private server written in C. Windows only, GCC via MinGW64.
- Build: `./build_and_run.bat` from MSYS MinGW64 shell at `D:/h1z1server/yeah/H1Z1-C-Server`
- Login server: port 20042 | Zone server: port 60000
- Two separate console windows — login server launches in new window, zone server in current window
- Crash logs: `C:\Users\9\AppData\Local\Temp\SCE\wws_crashreport\H1Z1.exe-*.session`
- Zone logs: `H1Z1-C-Server\logs\zone_*.log` (via tee in build_and_run.bat)
- Client version: `C:\Program Files (x86)\Steam\steamapps\content\app_433850\depot_433851\H1Z1.exe`
- DInput8 hook injected into client provides internal debug logging to `GameMessages.log`

## Large File Protocol (Token Saving — CRITICAL)
- **DO NOT** read `schema/output/client_protocol_1087.c` — 10,000+ lines, explicitly blocked
- **DO NOT** read `src/schema_tool.c` — ~3000 lines, never modified
- **DO NOT** read `src/utils/endian.c` — ~1500 lines of lookup tables, never modified
- **DO NOT** read `data/*.bin` files — binary blobs
- If you need a struct/opcode from `client_protocol_1087.c`, **STOP and ask the user** to fetch specific lines
- When reading logs, always use `grep` — never `cat` on zone logs (can be 15MB+ from GameTimeSync spam)

## Architecture
- `src/kotk_login_server.c` / `src/kotk_zone_server.c` — entry points, hot-reloadable DLLs
- `src/win32_login_server.c` / `src/win32_zone_server.c` — platform layer
- `src/zone/onLogin.c` — main login sequence, `DeployCharacter()`, `OnLogin()`
- `src/zone/sendSelfToClient.c` — full character data packer
- `src/zone/gatewayApi.c` — gateway protocol, channel routing, ticket parsing
- `src/zone/zonePacketHandler.c` — incoming zone packet dispatcher
- `src/zone/clientProtocol_1087.c` — `ZonePacketSend()`, `ZonePacketRawFileSend()`
- `src/zone/clientProtocol_1087.h` — struct definitions (BaseItem, ItemDefinition, ItemData)
- `src/soe/session.h` — SessionState struct
- `src/soe/outputStream.c` — OutputStreamWrite(), fire-and-forget, no retransmit buffer
- `src/soe/coreProtocol.c` — SOE packet handler
- `schema/output/kotk_login_udp_11.c` — login protocol schema
- `data/` — raw binary blobs sent via ZonePacketRawFileSend()

## Key Functions
- `ZonePacketSend(app, session, arena, Kind, &packet)` — send structured packet
- `ZonePacketRawFileSend(app, session, arena, maxLen, path)` — send raw bin file synchronously
- `GatewayTunnelDataSend(app, session, buf, totalLen)` — wraps zone data in gateway tunnel header
- `DeployCharacter(app, session)` — called from OnLogin(), sends character sequence
- `TunnelDataHeaderLen` — header size prepended before zone packet data

## Current Login Sequence (onLogin.c)
`InitializationParameters` → `SendZoneDetails` → `ClientGameSettings` → `UpdateWeatherData` → `ClientUpdate_UpdateLocation` (trigger_loading_screen=FALSE) → `ClientBeginZoning` (wait_for_zone_ready=FALSE, cloudFile="sky_Z_clouds.dds") → `ClientInitializationDetails` → `SendSelfToClient` → [5 small bin files] → `ContainerInitEquippedContainers` → `Equipment_SetCharacterEquipment` → `Loadout_SetLoadoutSlots` → `Character_CharacterStateDelta` → `GameTimeSync` → `ClientUpdate_DoneSendingPreloadCharacters` → `ZoneDoneSendingInitialData` → `ClientUpdate_NetworkProximityUpdatesComplete` → `AddLightweightPc` → `broadcast.bin`

## Raw Bin Files (data/) — sent after SendSelfToClient in order
1. `Command.ItemDefinitions.bin` (~5KB, ~10 fragments)
2. `ReferenceData.WeaponDefinitions.bin` (~40KB, ~81 fragments)
3. `ReferenceData.ProjectileDefinitions.bin` (~6KB, ~12 fragments)
4. `ReferenceData.ProfileDefinitions.bin` (~1KB, 1 fragment)
5. `ReferenceData.ItemClassDefinitions.bin` (~4KB, ~8 fragments)
6. `ReferenceData.DynamicAppearance.bin` (6.3MB, ~13,004 fragments) — THIS IS THE PROBLEM FILE

## Current Problem — The Root Cause
The client is stuck in `cClientRunStateWaitForConfirmationPacket` (state 22). It will not send `ClientIsReady` (0x64) until all five flags are 1:


WaitForWorldReady Status: IsZoneReady=1 HaveProxiedCharacter=0 HaveProxiedActor=0 ActorReadyToDraw=0 WeatherDataSynced=0
x=0.000000, y=0.000000, z=0.000000


Only `IsZoneReady=1`. All others are 0.

**Why:** `ReferenceData.DynamicAppearance.bin` (6.3MB) is sent as part of DeployCharacter in a single synchronous burst (~13,004 UDP fragments). The client's UDP receive buffer (~500 packets) overflows. The OS silently drops packet at sequence ~638. The client receives sequences 639–1021+ but flags them all as OutOfOrder (0x0011) because 638 never arrived. Since SOE reliable delivery is strictly ordered, the client freezes — it will not process ANY packet after the gap. Every packet sent after sequence 638 (UpdateWeatherData, SendSelfToClient, ContainerInit, everything) sits unprocessed in the client's buffer forever. Hence all four flags stay 0.

**Confirmed via:** 764 unhandled OutOfOrder (0x0011) packets in zone log, sequences 638–1021 contiguous. Zero sendto() failures — packets were handed to OS successfully but OS dropped them on the receive side. GameMessages.log confirms flags never change.

**The character model that renders** is from `AddLightweightPc` (visual-only lightweight entity), NOT from `SendSelfToClient` (the real ProxiedCharacter). Position 0,0,0 in the flags confirms `ClientUpdate_UpdateLocation` was also never processed.

## The Fix — Two-Phase DeployCharacter (IN PROGRESS)
DynamicAppearance.bin must be delivered slowly over many ticks (~100 fragments/tick = ~130 ticks = ~3 seconds). All finalization packets that depend on DynamicAppearance being loaded must fire AFTER delivery completes.

### Phase 1 (fires immediately in OnLogin tick):
- SendSelfToClient
- Queue DynamicAppearance.bin for multi-tick delivery
- Set `session->pendingPhase2 = TRUE`
- Return immediately

### Phase 2 (fires the tick DynamicAppearance delivery completes):
- ContainerInitEquippedContainers
- Equipment_SetCharacterEquipment
- Loadout_SetLoadoutSlots
- Character_CharacterStateDelta
- GameTimeSync
- ClientUpdate_DoneSendingPreloadCharacters
- ZoneDoneSendingInitialData
- ClientUpdate_NetworkProximityUpdatesComplete

### AddLightweightPc and broadcast.bin stay in OnLogin after Phase 1 — lightweight, no dependency on DynamicAppearance.

### New infrastructure needed:
- `session->largeSendFile` (FILE*) — open file handle for streaming
- `session->largeSendFragsPerTick` (u32) — fragments to send per tick
- `session->largeSendActive` (b32) — TRUE while sending
- `session->pendingPhase2` (b32) — TRUE while waiting for Phase 2
- `ZonePacketQueueLargeFile(app, session, path, fragsPerTick)` — opens file, sets active
- `ZonePacketDrainLargeSend(app, session, arena)` — called every tick, sends N fragments
- Drain loop check in `kotk_zone_server.c` after CorePacketHandle:
  ```c
  ZonePacketDrainLargeSend(app, &app->sessions[knownSession], &app->arenaPerTick);
  if (session->pendingPhase2 && !session->largeSendActive && session->largeSendFile == NULL) {
      session->pendingPhase2 = FALSE;
      DeployCharacterPhase2(app, &app->sessions[knownSession]);
  }
  ```
- Critical guard: `largeSendFile == NULL` prevents Phase 2 firing on same tick Phase 1 runs

### Current blocker on the fix:
`ZonePacketDrainLargeSend` is sending malformed packets — client reports "failed to unserialize a packet to process. 512 bytes ignored." The drain function must match exactly how `ZonePacketRawFileSend` prepares the buffer (TunnelDataHeaderLen bytes reserved at front, fread into buf+TunnelDataHeaderLen, then GatewayTunnelDataSend). Need to verify `GatewayTunnelDataSend` signature and what it expects.

## What Doesn't Work (DO NOT RETRY)
- `trigger_loading_screen = TRUE` on ClientUpdate_UpdateLocation — breaks zone load
- Empty cloudFile in ClientBeginZoning — breaks zone load, must be `sky_Z_clouds.dds`
- `wait_for_zone_ready = TRUE` — no effect
- Sending all packets on channel 5 — no effect
- Double-deploying SendSelfToClient on 0x11 0x97 — no effect
- Sending DynamicAppearance.bin synchronously — overflows UDP buffer, causes 764 OOO packets
- Character_UpdateCharacterState state1=0x60 — unknown if correct, not yet confirmed
- Phase 2 split implemented by Claude Code previously — broke login by putting zone functions in kotk_login_server.c and firing Phase 2 on same tick as Phase 1

## Login Flow (confirmed working)
1. Client connects to login server port 20042
2. Login server handles: LoginRequest → ServerListReply → CharacterSelectInfoRequest → CharacterSelectInfoReply → NameValidation (tunnel 0xa7) → CharacterCreateRequest → CharacterCreateReply → CharacterLoginRequest (0x07) → CharacterLoginReply (sends redirect to 127.0.0.1:60000)
3. Client connects to zone server port 60000
4. Zone server handles: gateway handshake → ticket validation → OnLogin() fires

## Session State (confirmed)
- `lastLoginDate` must be non-zero in CharacterSelectInfoReply for client to show Play button
- CharacterLoginReply fields: server_address="127.0.0.1:60000", server_ticket="7y3Bh44sKWZCYZH:{characterName}", encryption_key=RC4 key, soe_protocol_version=3

## SOE Protocol Notes
- CRC disabled (crcLen=0), compression disabled, encryption enabled after gateway login
- RC4 key (base64): `F70IaxuU8C/w7FPXY1ibXw==`
- Channels 0, 1, 2, 4, 5 supported — all zone packets sent on channel 0
- MAX_PACKET_LENGTH=512, fragments used for anything larger
- OutputStreamWrite is fire-and-forget — NO retransmit buffer exists
- OutOfOrder handler (0x0011) does not exist in coreProtocol.c — server ignores all OOO notifications
- sequence++ happens before sendto() — latent bug if sendto() ever fails

## Stability Configuration
- MAX_SESSIONS_COUNT=1 (zone), 2 causes fragment pool issues
- MAX_FRAGMENTS=12000
- Arena: default sizes
- Channel 1/2/4/5 fragment pools: 64

## VCPatcher.cpp (reference only, not compiled)
Client state machine enum:
- cClientRunStateWaitForFirstZone = 21
- cClientRunStateWaitForConfirmationPacket = 22
- cClientRunStateRunning = 25
- cClientRunStateWaitForZoneLoad = 27
Client handleIncomingPackets address: 0x140EEDE90

## GameMessages.log (DInput8 hook output)
Located in game directory. Contains:
- TransitionClientRunState — client state machine transitions
- WaitForWorldReady Status — the 5 flags + position, polls every ~1 second
- RECEIVED= lines — zone packets the client successfully parsed
Key flag line to watch: `WaitForWorldReady Status: IsZoneReady=X HaveProxiedCharacter=X HaveProxiedActor=X ActorReadyToDraw=X WeatherDataSynced=X`
```