# H1Z1-C-Server — Claude Context

## Project
* H1Z1: King of the Kill (v0.23.4.161178) private server in C. Windows only, GCC via MinGW.
* Build: `./build_and_run.bat` from `D:/h1z1server/yeah/H1Z1-C-Server`
* Login server: port 20042 | Zone server: port 60000

## Large File Protocol (Token Saving)
* **DO NOT** attempt to read, `cat`, or search `schema/output/client_protocol_1087.c`. It is over 10,000 lines long and is explicitly blocked to prevent token waste.
* If you need an exact struct, opcode, or packing definition for a specific packet from that file, **STOP and ask the user** to fetch the relevant lines for you.
* **DO NOT** read `src/schema_tool.c` — it is ~3000 lines of parser code that is never modified.
* **DO NOT** read `src/utils/endian.c` — it is ~1500 lines of lookup tables and endian helpers that are never modified.
* The five raw `.bin` files in `data/` are binary blobs, do not attempt to read them.

## Current Status
Client reaches `cClientRunStateWaitForConfirmationPacket` (state 22). Character model renders and animates at correct world position. `ProxiedCharacter` is `nullptr` — not yet controllable. Session stable for 85+ seconds. Client only sends `GameTimeSync` heartbeats, never `ClientFinishedLoading` or `ClientIsReady`.

## Architecture
* `src/kotk_login_server.c` / `src/kotk_zone_server.c` — entry points, hot-reloadable DLLs
* `src/zone/onLogin.c` — main login sequence + `DeployCharacter()`
* `src/zone/sendSelfToClient.c` — full character data packer
* `src/zone/gatewayApi.c` — gateway protocol, channel routing, ticket parsing
* `src/zone/zonePacketHandler.c` — incoming zone packet dispatcher
* `src/soe/` — SOE protocol (sessions, streams, fragments, core protocol)
* `schema/` — `.schm` files compiled to `schema/output/*.c` by `schema_tool.exe`
* `data/` — raw binary blobs sent via `ZonePacketRawFileSend()`

## Key Functions
* `ZonePacketSend(app, session, arena, Kind, &packet)` — send structured packet
* `ZonePacketRawFileSend(app, session, arena, maxLen, path)` — send raw bin file
* `DeployCharacter(app, session)` — called from `OnLogin()`, sends full character sequence
* `GatewayTunnelDataSend()` — wraps zone packets in gateway tunnel header

## Login Sequence (current)
`InitializationParameters` -> `SendZoneDetails` -> `ClientGameSettings` -> `UpdateWeatherData` -> `ClientUpdate_UpdateLocation` -> `ClientBeginZoning` -> `ClientInitializationDetails` -> `SendSelfToClient` -> [5x raw bin files] -> `ContainerInitEquippedContainers` -> `Equipment_SetCharacterEquipment` -> `Loadout_SetLoadoutSlots` -> `Character_CharacterStateDelta` -> `GameTimeSync` -> `ClientUpdate_DoneSendingPreloadCharacters` -> `ZoneDoneSendingInitialData` -> `Character_UpdateCharacterState` -> `ClientUpdate_NetworkProximityUpdatesComplete` -> `AddLightweightPc` -> `broadcast.bin`

## Raw Bin Files (data/)
Sent after `SendSelfToClient`, in this order:
1. `Command.ItemDefinitions.bin` (5KB)
2. `ReferenceData.WeaponDefinitions.bin` (40KB)
3. `ReferenceData.ProjectileDefinitions.bin` (6KB)
4. `ReferenceData.ProfileDefinitions.bin` (1KB)
5. `ReferenceData.ItemClassDefinitions.bin` (4KB)
*Note: Generated from QuentinGruber/h1z1-server TypeScript server at startup. Adding these moved the client from state 27 to state 22.*

## What Doesn't Work (DO NOT RETRY)
* `trigger_loading_screen = TRUE` on `ClientUpdate_UpdateLocation` — breaks zone load
* Empty cloudFile in `ClientBeginZoning` — breaks zone load, must be `sky_Z_clouds.dds`
* `wait_for_zone_ready = TRUE` — no effect
* Sending on channel 5 — no effect
* Double-deploying `SendSelfToClient` on `0x11` `0x97` — no effect

## Next Step (High Confidence)
Client is stable in `cClientRunStateWaitForConfirmationPacket` (state 22) for 237+ seconds.
Character model renders and animates at correct position. Session no longer crashes.
The confirmation packet that transitions state 22 → state 25 (Running) has not been identified yet.
Check the zone log for what packets the client sends after the initial deployment —
specifically whether `ClientIsReady` (0x64) is being received and handled, and what
other unhandled opcodes appear. The `ZONE_CLIENTISREADY_ID` handler was updated to send
`DoneSendingPreloadCharacters`, `ZoneDoneSendingInitialData`, and `NetworkProximityUpdatesComplete`
in response — confirm these are firing. If `ClientIsReady` is not arriving, the client is
waiting for something else before it will send it.

## Recent Stability Fixes
- `MAX_SESSIONS_COUNT` increased to 2 — prevents fragment pool corruption on reconnect
- `MAX_FRAGMENTS` increased to 30000, arena to MB(200), perTick to MB(50)
- Channel 1/2/4/5 fragment pools increased from 64 to 1024

## Worth Considering (Lower Confidence)
* `Character_UpdateCharacterState` `state1=0x60` is a guess — check TypeScript server for real values.
* `ClientBeginZoning` may not belong in the sequence at all — TypeScript `sendInitData` doesn't include it.
* `Loadout.SetCurrentLoadout` vs `Loadout.SetLoadoutSlots` — may be wrong packet for this stage.

## Crash Log Location
`C:\Users\<user>\AppData\Local\Temp\SCE\wws_crashreport\H1Z1.exe-*.session`
*Key fields: Client State, Character Pointer, Last Actor Updating*

## SOE Protocol Notes
* CRC disabled (`crcLen=0`), compression disabled, encryption enabled after gateway login.
* RC4 key (base64): `F70IaxuU8C/w7FPXY1ibXw==`
* Channels 0, 1, 2, 4, 5 supported — all zone packets currently sent on channel 0.
* `MAX_PACKET_LENGTH=512`, fragments used for anything larger.