# H1Z1 KotK C Server Emulator — Complete Project Status

**Date:** March 30, 2026  
**Project Path:** `D:\h1z1server\yeah\H1Z1-C-Server\`  
**Target:** H1Z1: King of the Kill, Preseason 3, Protocol 1087  
**Author/Developer:** Nick (with Z2Doggo as original repo author)

---

## What This Project Is

A from-scratch C server emulator for H1Z1 King of the Kill (Preseason 3). The client is the retail KotK executable using protocol version 1087. The server handles the full connection lifecycle: login server on port 20042, zone server on port 60000, SOE protocol layer (session management, fragmentation, RC4 encryption, multi-channel reliable delivery), gateway protocol (login, tunnel, channel routing), and the zone protocol (game packets).

The codebase uses a custom schema tool (`src/schema_tool.c`) that reads `.schm` files and auto-generates C packet packers/unpackers. The two schema files are `kotk_login_udp_11.schm` (login protocol) and `client_protocol_1087.schm` (zone protocol). Generated output lands in `schema/output/`.

The server architecture is DLL hot-reload based: `win32_zone_server.c` loads `zoneModule.dll` (compiled from `kotk_zone_server.c`) and calls `serverTick()` at 45Hz. Same pattern for the login server.

---

## Current State (What Works)

The character **loads into the game world**. Terrain renders (Z2 map), weather works, UI is functional, WASD movement works, and the camera is properly attached to the character. The character model **does render** — a naked male survivor with underpants is visible in third person. First-person arms/hands appear when the model renders. Equipment attachments (head, hair, chest, legs, eyes, first-person arms) are confirmed processing by client logs.

The most recent test showed the character loading successfully with the restructured h1emu-matching packet sequence. Movement was functional but had a speed-stacking bug (documented below under "Last Known Bug").

---

## The Three Original Problems & Their Status

**1. Invisible Character Model — MOSTLY FIXED (intermittent)**  
The character now renders. It appeared once as a naked male in third person during earlier testing, and loaded again in the most recent test after the h1emu sequence restructure. The intermittency appears to be caused by the deploy loop bug (26× repeated DeployCharacter calls) — fixing that should make rendering consistent.

**2. Slow Movement / No Sprint — ADDRESSED**  
The h1emu restructure moved `Command.RunSpeed` and `ClientUpdate.ModifyMovementSpeed` to `ClientFinishedLoading` (matching h1emu's sequence). However, the deploy loop bug caused `ModifyMovementSpeed` to be sent 48 times, stacking speed until the character flew across the map. Once the loop is fixed, speed should normalize.

**3. Empty Inventory — NOT YET FIXED**  
Fists and binoculars are sent in `Loadout.SetLoadoutSlots` and `equipment_slots` inside `SendSelfToClient`, but the inventory UI still shows nothing. This likely requires populated `items1` array in `SendSelfToClient` or `ClientUpdate.ItemAdd` packets, which h1emu sends later in the flow.

---

## Last Known Bug (Must Fix First)

**DeployCharacter infinite loop.** The `DeployCharacter()` function resets `session->isReady = FALSE` and `session->finished_loading = FALSE` at its top. This means every subsequent `ClientIsReady` packet from the client passes the guard check and triggers another full deploy cycle. In the last test, this caused **26 deploy cycles** and **48 equipment/movement sends**, explaining the "few minutes to load" and the speed-stacking flyaway.

**The fix:** Remove these two lines from the top of `DeployCharacter()`:
```c
// REMOVE THESE:
session->finished_loading = FALSE;
session->isReady = FALSE;
```

These flags are already correctly reset in `OnLogin()` before `ClientBeginZoning` (which is the right place — when a genuine new zone starts). They should NOT be reset inside `DeployCharacter`.

---

## Authoritative Packet Sequence (from h1emu debug logging)

This was captured by running h1emu's Just Survive server (protocol 1080) with `$env:DEBUG="ZoneServer"` and extracting every `send data` and `Receive Data` line. This is the proven working sequence.

### Phase 1 — OnLogin (sendInitData, before ClientIsReady):

| # | Packet | Notes |
|---|--------|-------|
| 1 | `InitializationParameters` | environment="LIVE_KOTK" |
| 2 | `SendZoneDetails` | zone="Z2", zoneType=4 |
| 3 | `ClientGameSettings` | timescale=1.0, enableWeapons=1 |
| 4 | `ReferenceData.DynamicAppearance` | Skin tones only (3 entries, opcode 0x1706) |
| 5 | `SendSelfToClient` | **Sent ONCE here only — never re-sent** |
| 6 | `Container.InitEquippedContainers` | Empty (0 containers) |
| 7 | *(raw cached data)* | ItemDefinitions, WeaponDefinitions, ProfileDefinitions, ProjectileDefinitions, ItemClassDefinitions — sent as raw binary blobs |
| 8 | `ClientBeginZoning` | Triggers zone load |
| 9 | `ClientUpdate.UpdateLocation` | trigger_loading_screen=TRUE |
| 10 | `ClientInitializationDetails` | unk_u32_1=1 |

The client then sends: `WallOfData.UIEvent`, `SetLocale`, `ClientInitializationDetails`, `GetContinentBattleInfo`, `GetRewardBuffInfo`, `GameTimeSync` (multiple).

### Phase 2 — DeployCharacter (on ClientIsReady):

| # | Packet | Notes |
|---|--------|-------|
| 1 | `AddSimpleNpc` × many | World objects + Replication data (we skip — no world objects yet) |
| 2 | `POIChangeMessage` | Empty/no fields |
| 3 | `Character.UpdateCharacterState` | 8 state bytes (all 0) + game_time |
| 4 | `ClientUpdate.DoneSendingPreloadCharacters` | is_done=TRUE |
| 5 | `DtoObjectInitialData` | **TODO — opcode unknown, not implemented yet** |
| 6 | `Character.CharacterStateDelta` | guid_1=characterId, guid_3=0x40000000 |
| 7 | `ZoneDoneSendingInitialData` | **IMMEDIATE** — client exits loading screen |
| 8 | *(5 second gap)* | |
| 9 | `ClientUpdate.NetworkProximityUpdatesComplete` | **DEFERRED 5 seconds** |

**Critical: h1emu does NOT send `AddLightweightPc` for self, does NOT re-send `SendSelfToClient`, and does NOT send Equipment/Loadout here.**

### Phase 3 — ClientFinishedLoading:

| # | Packet | Notes |
|---|--------|-------|
| 1 | `UpdateWeatherData` | Same weather params |
| 2 | `Character.WeaponStance` | stance=1 |
| 3 | `Equipment.SetCharacterEquipment` | Chest + Legs + Fists with .adr models |
| 4 | `Command.RunSpeed` | runSpeed=0.0 |
| 5 | `ClientUpdate.ModifyMovementSpeed` | speed=2.0, movementVersion=1 |

### Phase 4 — Later (client-initiated):

The client sends `Character.FullCharacterDataRequest` (~3s after loading). H1emu responds with `LightweightToFullNpc` (opcode 0xdb), **not** `LightweightToFullPc` (0xda).

---

## Key Functions (The Big Three)

### `OnLogin()` — `src/zone/onLogin.c`

Called when the gateway receives a login request and the character ID is resolved. Sends the entire Phase 1 sequence: initialization params, zone details, game settings, reference data, `SendSelfToClient`, container init, all raw `.bin` reference files, then `ClientBeginZoning` to trigger the zone load. This is the entry point for everything.

### `DeployCharacter()` — `src/zone/onLogin.c`

Called when the client sends `ClientIsReady` (opcode 0x04) after finishing zone load. Sends Phase 2: `POIChangeMessage`, `Character.UpdateCharacterState`, `DoneSendingPreloadCharacters`, `CharacterStateDelta`, `ZoneDoneSendingInitialData` (immediate), and schedules `NetworkProximityUpdatesComplete` for 5 seconds later. Does NOT send equipment, loadout, or movement — those moved to `SendEquipmentAndMovement()`.

### `SendSelfToClient()` — `src/zone/sendSelfToClient.c`

Dynamically builds the massive `SendSelfToClient` packet (opcode 0x03) with a `stream:u32` length prefix. Contains: character GUID, actorModelId (9469=male, 9474=female), head/hair models, position, rotation, identity (name), profile (id=5, type=3), loadout (id=17 for KotK, fists slot 7 + binoculars slot 5), 3 equipment slots (chest/legs/fists with .adr models), 9 resources (health/hunger/hydration/stamina etc.), and dozens of empty arrays for quests/achievements/recipes/etc. Uses the schema-generated packer, then sends via `ZonePacketSendSelfDebug()` which hex-dumps the first 128 bytes for verification.

---

## Key Discoveries Made Across Sessions

### Reference Data Files Were Not Loading
All `ZonePacketRawFileSend` calls with relative paths (`"data/..."`) were failing silently with Error 3 because the DLL runs from a different working directory than the project root. **Fixed by using paths relative to the DLL's location** (the current code uses `"data/..."` which works when CWD is correct). If paths break again, use absolute paths like `"D:\\h1z1server\\yeah\\H1Z1-C-Server\\data\\..."`.

### DynamicAppearance Packet Built from Scratch
`ReferenceData_DynamicAppearance.bin` (162 bytes, opcode 0x1706) was hand-built with 3 shader parameter entries: CharSkinTone (ID 122), CharHairColor (ID 125), CharEyeColor (ID 129). Sent before `SendSelfToClient` in OnLogin matching h1emu's order.

### Protocol 1087 Uses u64 for last_login_date
The Z1BR reference binary uses u32 for this field (different protocol version). Changing to u32 broke everything (`HaveProxiedCharacter=0`). **u64 is confirmed correct for protocol 1087.**

### character_stats1 Crashes the 1087 Client
Populating the `character_stats1` array in `SendSelfToClient` with movement stats causes the KotK client to crash. The array is currently sent empty (count=0). H1emu's Just Survive server also works with empty stats.

### KotK Uses Loadout ID 17, Not 3
Just Survive uses loadout ID 3. KotK Preseason 3 uses loadout ID 17, sourced from `Loadouts.LoadoutSlotDefinitionDataSource.rtf`. Constants defined in `core_base_full_character.h`.

### Deferred NetworkProximityUpdatesComplete
Uses `__time64_t` wall-clock time (not tick count, which was a high-frequency counter making delays instant). `ZoneDoneSendingInitialData` sent immediately, only `NetworkProximityUpdatesComplete` deferred by 5 seconds. Implemented in the `serverTick()` loop in `kotk_zone_server.c`.

### FullCharacterDataRequest Response
Client sends `Character.FullCharacterDataRequest` (opcode 0x0f45) ~3 seconds after `ClientFinishedLoading`. H1emu responds with `LightweightToFullNpc` (0xdb), not `LightweightToFullPc` (0xda). Our server now sends 0xdb.

### SOE Multi-Channel Support
The server supports SOE channels 0-5 with independent fragment pools, input/output streams, RC4 state, and ack tracking per channel. Channel 0 is the main data channel. Channel 1 carries some client packets (like `LobbyGameDefinition.DefinitionsRequest`). Channels are identified by the SOE packet ID: `CoreDataId + channel * 0x10`.

### Gateway Channel Routing
The gateway byte format is `(channel << 5) | packetId`. Channel 0 carries control packets (LoginRequest/Reply) and tunnel data. Non-zero channels route directly as tunnel data. The character name is extracted from the server ticket (`"7y3Bh44sKWZCYZH:CharacterName"`) during gateway login.

---

## File Layout

| File | Purpose |
|------|---------|
| `src/zone/onLogin.c` | `OnLogin()`, `DeployCharacter()`, `SendEquipmentAndMovement()` |
| `src/zone/sendSelfToClient.c` | `SendSelfToClient()` dynamic packer + helpers |
| `src/zone/zonePacketHandler.c` | All incoming zone packet handlers |
| `src/zone/clientProtocol_1087.c` | `ZonePacketSend()`, `ZonePacketRawFileSend()`, position parsing, static view |
| `src/zone/clientProtocol_1087.h` | Item/weapon data structures |
| `src/zone/gatewayApi.c` | Gateway pack/unpack, tunnel routing, channel routing |
| `src/zone/gatewayApi.h` | Gateway packet IDs and structures |
| `src/kotk_zone_server.c` | Main server loop, session init, deferred tick logic |
| `src/kotk_login_server.c` | Login server main loop |
| `src/login/loginPacketHandler.c` | Login packet dispatch |
| `src/login/loginUdp_11.c` | Login send helpers, name validation, character create/select |
| `src/soe/coreProtocol.c` | SOE session/data/ack/fragment handling, all 6 channels |
| `src/soe/inputStream.c` | Reliable ordered delivery, RC4 decrypt, fragment assembly |
| `src/soe/outputStream.c` | Reliable send, RC4 encrypt, fragmentation |
| `src/core/entities/core_base_full_character.h` | KotK constants (loadout IDs, item enums, equipment slots) |
| `schema/client_protocol_1087.schm` | Zone protocol schema |
| `data/*.bin` | Raw reference data packets (ItemDefinitions, WeaponDefinitions, etc.) |

---

## What's Been Tried and Confirmed NOT the Problem

| Thing Tried | Result |
|-------------|--------|
| Wrong actorModelId (9240 vs 9469) | 9469 confirmed correct — model DID render |
| Wrong last_login_date size (u32 vs u64) | u64 confirmed for protocol 1087 |
| character_stats1 with movement stats | Crashes KotK 1087 client |
| Command.RunSpeed alone | Doesn't fix speed by itself |
| ActivateProfile packet | No visible effect |
| ModifyMovementSpeed alone | No effect without proper sequence |
| Missing equipment attachments | Client now processes them correctly |
| Wrong loadout ID (3 vs 17) | 17 is correct for KotK |
| AddLightweightPc for self | H1emu doesn't send it — removing had no negative effect |
| Re-sending SendSelfToClient in DeployCharacter | H1emu doesn't do this — removing is correct |

---

## Remaining TODO (Priority Order)

1. **Fix DeployCharacter loop** — Remove `isReady`/`finished_loading` reset from `DeployCharacter()`. This should make character loading consistent and fast, and fix the speed stacking.

2. **Test with loop fix** — Verify character renders consistently, movement is normal speed, no more flyaway.

3. **Investigate DtoObjectInitialData** — H1emu sends this in ClientIsReady but we don't. Need to find the opcode and packet format.

4. **Inventory items** — Populate `items1` array in `SendSelfToClient` or send `ClientUpdate.ItemAdd` packets to give the player fists and binoculars in inventory.

5. **Sprint** — May require correct `Command.RunSpeed` value (currently 0.0) or a specific resource/stat configuration.

6. **ModifyMovementSpeed tuning** — Currently sending speed=2.0, movementVersion=1. May need adjustment for KotK 1087.

7. **World objects** — `AddSimpleNpc` + `Replication.CreateRepData` + `Replication.CreateComponent` for Fort Destiny props, vehicles, etc.

8. **Items.AccountItemManagerStateChanged** — H1emu sends this after `ZoneDoneSendingInitialData`. Unknown format.