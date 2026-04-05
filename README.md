# H1Z1-C-Server — An `H1Z1: King of the Kill` server made in C

**Target:** H1Z1: King of the Kill, Preseason 3, Protocol 1087  
**Client Build:** March 7, 2017 (depot 433851)  
**Developer:** Nikvrasu (original repo: Z2Doggo)

---

## Credits

- Rhett (Indie/H1Emu — Made the server to begin with)
- LegendsNeveerrDie (Indie)
- GhostsKappa (On Break — H1Emu)
- Z2Doggo (Original Author of this C-Server (https://github.com/Z2Doggo/H1Z1-C-Server) — H1Emu)

## Contributions

Thank you to Jacob Seidelin and QuentinGruber without whom none of this would have been possible. Also thank you to everyone over at the [H1Emu Project](https://github.com/QuentinGruber/h1z1-server) for helping us learn many great things.

---

## Current Status

### Milestone Checklist

- [x] Compile for use, send and receive packets
- [x] Handle client connection request (BaseApi is setup & connected)
- [x] Send Session, Fingerprint and Encryption
- [x] Make sure the client gets the right data to get to the server list screen
- [x] Zone in to the game with a character
- [x] Terrain, weather, WASD movement working
- [x] Character model renders (naked male survivor, default state)
- [x] Clothing visually renders on character model
- [ ] FP arms appear in first person
- [ ] Invisibility bug resolved (character visible without wall trick)
- [ ] Inventory slot grid UI working
- [ ] Health bar / HUD resources showing
- [ ] Match state initialized (player count, gas timer, countdown)
- [ ] Reverse all vital packets to function as in 2017

### What Works

- Client connects, authenticates, and loads into the Z2 map
- Terrain renders correctly, weather system works
- WASD movement is functional
- Character model renders in third person — correct naked male survivor with white underpants (default state)
- All 5 `ProcessNewAttachment` callbacks fire: head, chest, legs, weapon slot, eyes
- Clothing visually renders on the character model (hoodie, jeans, sneakers) via `SetCharacterEquipment` + `ActivateProfile`
- Physics, collision, and animation work correctly
- Compass and ping display (top right) work
- Compass direction (NNE A6) shows correctly

### What Doesn't Work

- **Character is invisible on spawn** — requires the "wall trick" (backing camera into geometry) to become visible. This is the primary unresolved bug. See dedicated section below.
- **Inventory slot grid is missing** — the equipped silhouette panel shows no item slots around the mannequin. Clothing items do not appear in the inventory UI despite rendering on the 3D model.
- **Health bar missing** — resource data is sent but the HUD health bar does not appear.
- **Match state not initialized** — player/team count, gas timer, and countdown UI do not appear. The game never enters "match started" state.
- **`FullCharacterDataRequest` never sent by client** — the character stays as a lightweight entity permanently with no upgrade to full PC entity (Not yet sure if even necessary).

---

## Dependencies

The game requires the following DirectX and Visual C++ libraries to be installed.

- VC 2015 Redist — [Download from Microsoft](https://www.microsoft.com/en-gb/download/details.aspx?id=48145)
- DirectX June 2010 Redist — [Download from Guru3D](https://www.guru3d.com/files-get/directx-end-user-runtimes-(june-2010),8.html)

---

## How to Download the Client

> **WARNING:** This will only work if you have `Z1: Battle Royale` (formerly `H1Z1: King of the Kill`) in your Steam library.

Use the latest version of [DepotDownloader](https://github.com/SteamRE/DepotDownloader).

**AppID:** 433850 | **DepotID:** 433851 | **ManifestID:** 6098349229565958949

```
dotnet .\DepotDownloader.dll -app 433850 -depot 433851 -manifest 6098349229565958949 -username YOURUSERNAME -password YOURPASSWORD
```

---

## Setup ClientConfig.ini

In the H1Z1 folder, open `ClientConfig.ini`.

Remove the line:
```
Server=Lvshaa-liv-l02.h1z1.net:20042;...
```

Add the following at the top of the file:
```
World=None
Server=127.0.0.1:1115
usenewui=1
SessionId=0
```

---

## Building the Server

Make sure you have the GNU C Compiler (GCC) installed. If not, see this [install tutorial](https://www.youtube.com/watch?v=8CNRX1Bk5sY).

- Double-click `build_kotk_login.bat` to build the Login Server
- Double-click `build_kotk_zone.bat` to build the Game Server
- OR click `build_and_run.bat` to build and launch both (reccommended)

## Launching the Server

Double-click `run_all.bat` in the `H1Z1-C-Server` folder to run both the login server and zone server.

---

## The Invisibility Bug — Full Analysis

### Symptom

The character mesh, shadow, and all rendered geometry are absent on initial spawn. The physics capsule, movement, animation, and FP arms all work correctly. There is no shadow. The 3D model only becomes visible after the camera clips into world geometry (the "wall trick").

### What the Wall Trick Does

The trigger is the **camera contacting geometry**, not the physics capsule. Testing confirmed:
- Walking the physics capsule into a wall with no camera clip = no visibility change
- Camera clipping into geometry = character immediately becomes visible with correct mesh and shadow
- Going into first person after becoming visible = invisible again
- Returning to third person = requires wall trick again

This is a camera-driven re-evaluation of the character's render state, not a physics event or a server packet response.

### What Has Been Ruled Out

| Hypothesis | Result |
|---|---|
| Wrong `actorModelId` | 9469 confirmed correct |
| Wrong `transient_id` mismatch between `SendSelfToClient` and `AddLightweightPc` | Both encode value `1` as `0x04` via `uint2b` — they match |
| Position wrong in `AddLightweightPc` | Confirmed correct: `(-297.31, 506.06, -4894.10)` via hex decode |
| LOD culling / `ClientConfig_en_US.ini` missing | Copied file, increased LodBins distances — no effect |
| `tp_camera_distance = 0` in weapon definitions | Sent full fists weapon definition with `tp_camera_distance = 3.5f` — no effect |
| `Character_UpdateCharacterState` visible flag | Sent all 7 state bytes = 1 (including visible bit) — no effect |
| `UpdateCamera` (0x57) trigger packet | Sent — no effect |
| `loadoutId = 3` in `CharacterSelectInfoReply` | Fixed to `loadoutId = 17` — no effect on visibility |
| `SendSelfToClientRaw` (working captured binary) | Caused `HaveProxiedCharacter=0`, client never progressed |
| `FullCharacterDataRequest` / `LightweightToFullPc` | Client never sends `0x0f 0x45` — h1emu's fix doesn't apply here |
| Channel 2 position data being dropped | Bug confirmed and fixed (removed early return) — no effect on visibility |
| Packet handler 1-byte loop broken | Confirmed dead code but all active handlers use single-byte opcodes — not the cause |

### Current Best Theory

The character mesh starts in an uninitialized render state. The client requires a specific server-triggered transition to commit the mesh to the scene graph. The wall trick accidentally triggers this via a camera distance recalculation. The exact packet or field that should trigger this proactively on spawn has not been identified.

The most productive remaining path is **binary diffing `sendself_patched.bin` against the dynamic packer output** — both are 18,815 bytes with only 7 bytes different (all in the patched header fields). If the raw binary makes the character visible when the dynamic packer does not, a field in the dynamic packer is wrong.

---

## Authoritative Packet Sequence

### Phase 1 — OnLogin (before ClientIsReady)

| # | Packet | Notes |
|---|---|---|
| 0 | `Character_UpdateCharacterState` | All states = 0 (hide during loading) |
| 1 | `InitializationParameters` | environment=`LIVE_KOTK` |
| 2 | `SendZoneDetails` | zone=`Z2`, zoneType=4 |
| 3 | `ClientGameSettings` | timescale=1.0, enableWeapons=1 |
| 4 | `ReferenceData_DynamicAppearance` | Raw binary, 3 skin tone entries |
| 5 | `SendSelfToClient` | Dynamic packer, sent once only |
| 6 | `AddLightweightPc` | Self-presence broadcast |
| 7 | `Character_UpdateScale` | scale=(1,1,1,1) |
| 8 | `ContainerInitEquippedContainers` | 5 containers (shirt/legs/feet/fists/binos) |
| 9 | `CommandItemDefinitions` | 5 item defs (85, 1542, 5747, 2178, 2216) |
| 10 | `ReferenceDataWeaponDefinitions` | Fists weapon def with camera values |
| 11 | `ClientBeginZoning` | Triggers zone load |
| 12 | `ClientUpdate_UpdateLocation` | trigger_loading_screen=TRUE |
| 13 | `ClientInitializationDetails` | unk_u32_1=1 |

### Phase 2 — DeployCharacter (on ClientIsReady)

| # | Packet | Notes |
|---|---|---|
| 1 | `POIChangeMessage` | Empty |
| 2 | `Character_UpdateCharacterState` | All 7 states = 1 (visible/active) |
| 3 | `ClientUpdate_DoneSendingPreloadCharacters` | is_done=TRUE |
| 4 | `UpdateCamera` (0x57) | Raw opcode byte, no fields |
| 5 | `DtoObjectInitialData` | Raw: `05 03 01 00 00 00 00 00 00 00 00 00 00 00` |
| 6 | `Character_CharacterStateDelta` | guid_1=charId, guid_3=0x40000000 |
| 7 | `ZoneDoneSendingInitialData` | Immediate — client exits loading screen |
| 8 | `ResourceEventBase` | 9 resources (health/hunger/hydration/stamina/virus/bleeding/comfort/fuel/condition) |
| 9 | `AccountItemManagerStateChanged` | Raw escrow packet |
| 10 | `Character_WeaponStance` | stance=0 |
| — | `ClientUpdate_NetworkProximityUpdatesComplete` | Deferred +5 seconds |

### Phase 3 — SendEquipmentAndMovement (on ClientFinishedLoading)

| # | Packet | Notes |
|---|---|---|
| 1 | `GameTimeSync` | time=300000, cycle_speed=0.0 |
| 2 | `UpdateWeatherData` | Full weather params |
| 3 | `Character_WeaponStance` | stance=0 |
| 4 | `Equipment_SetCharacterEquipment` | 8 slots: head/chest/legs/weapon/eyes/shirt/pants/feet |
| 5 | `ClientUpdate_ActivateProfile` | profile_id=4, 8 attachments, actor_model_id set |
| 6 | `Loadout_SetLoadoutSlots` | loadout_id=17, 5 slots (fists/binos/shirt/jeans/shoes) |
| 7 | `Command_RunSpeed` | run_speed=7.5f |
| 8 | `ClientUpdate_ModifyMovementSpeed` | speed=2.0, movementVersion=1 |

---

## Key Constants

```c
#define LOADOUT_ID_KOTK_CHARACTER   17
#define LOADOUT_SLOT_PRIMARY        1
#define LOADOUT_SLOT_SECONDARY      2
#define LOADOUT_SLOT_TERTIARY       4
#define LOADOUT_SLOT_BINOCULARS     5
#define LOADOUT_SLOT_MELEE          7
#define LOADOUT_SLOT_THROWABLES     9

#define ITEM_GUID_FISTS             0x0001000000000001ULL
#define ITEM_GUID_BINOCULARS        0x0001000000000002ULL

#define WEAPON_FISTS                85
#define WEAPON_BINOCULARS           1542

// Default spawn clothing
#define ITEM_DEF_HOODIE             5747   // Gas Runner Hoodie
#define ITEM_DEF_JEANS              2178   // Jeans (SkinnyLeg)
#define ITEM_DEF_SNEAKERS           2216   // Conveys Sneakers

// Default spawn clothing model files
// "SurvivorMale_Chest_Hoodie_Down.adr"
// "SurvivorMale_Legs_Pants_SkinnyLeg.adr"
// "SurvivorMale_Feet_Conveys.adr"

// Actor model IDs
// 9469 = SurvivorMale, 9474 = SurvivorFemale

// Equipment slot IDs (BaseClient.EquipmentSlots)
// 1=Head, 3=Chest, 4=Legs, 7=RightHand, 10=Shirt,
// 13=Feet, 14=Legs(clothing), 105=Eyeballs
```

---

## Character Data (SendSelfToClient)

The dynamic packer builds this packet with the following non-zero/non-empty fields:

- `guid` / `character_id` = session character GUID
- `transient_id.value` = 1 (encodes as `0x04` in uint2b wire format)
- `actor_model_id` = 9469 (male) or 9474 (female)
- `head_actor`, `hair_model` = gender-appropriate `.adr` filenames
- `head_id` = head type (1–8)
- `faction_id` = 2
- `position` = `(-297.31, 506.06, -4894.10, 1.0)`
- `rotation` = `(0.0, -0.7071, 0.0, 0.7071)`
- `character_first_name` / `character_name` = from session
- `profiles_count` = 1, `profile_id` = 4 (Unemployed — correct KotK base profile)
- `current_profile` = 4
- `loadout_id` = 17 (KOTK_CHARACTER)
- `loadout_slots_array_count` = 2 (fists at slot 7, binoculars at slot 5)
- `current_slot_id` = LOADOUT_SLOT_MELEE
- `character_resources_count` = 9 (health/hunger/hydration/stamina/virus/bleeding/comfort/fuel/condition)
- `is_admin` = TRUE (doesn't work yet though)
- `equipment_slots_count` = 0 (equipment sent via SetCharacterEquipment instead)
- `items1_count` = 0 (containers not yet implemented in SendSelf)
- `character_stats1_count` = 0 — **populating this array crashes the 1087 client**

---

## Inventory System — Current State

### Visual Rendering (Working)

Clothing items render on the 3D character model correctly via `Equipment_SetCharacterEquipment` + `ClientUpdate_ActivateProfile`. The `ProcessNewAttachment` callbacks fire for all 8 attachment slots.

### Inventory UI (Broken)

The equipped silhouette panel shows no item slot grid around the mannequin. The panel is completely empty — no slot boxes for head/chest/legs/feet/hands/weapon slots appear at all.

### What's Been Tried

- `ContainerInitEquippedContainers` with 5 containers (one per equipped item) — no effect on UI
- `CommandItemDefinitions` with 5 item definitions (correct `item_class`, `item_type`, `bulk` values) — no effect on UI
- `Loadout_SetLoadoutSlots` with loadout_id=17 and 5 slot entries — no effect on UI
- Changing `profile_id` from 3 to 4 everywhere — no effect on UI

### Root Cause Theory

The inventory slot grid UI is driven by the container system. The `contain_def_id` field in `ContainerInitEquippedContainers` is currently set to the item's def_id (5747, 2178, etc.) but should be a **container type definition ID** — a fixed value that maps to the "equipped slot container" type the client knows locally. The correct value is unknown and needs to be determined.

---

## Known Bugs in the Codebase

### Critical (active code paths)

| # | Issue | Location | Impact |
|---|---|---|---|
| 1 | `GetItemData` null pointer dereference | `clientProtocol_1087.c:261` | Crash on any call |
| 2 | `GetItemData` infinite loop | `clientProtocol_1087.c:243` | Hang if item is not weapon/armor/helmet |
| 3 | `outputStream3` RC4 not reset on reconnect | `coreProtocol.c:336` | Broken encryption on channel 3 after reconnect |
| 4 | Channel 2 early return removed | `gatewayApi.c:254` | **Fixed** — position data now routes correctly |
| 5 | Zone packet 1-byte matching loop | `zonePacketHandler.c:19` | **Dead code** — no active impact, all handlers use 1-byte opcodes |

### Medium (latent)

| # | Issue | Location |
|---|---|---|
| 6 | `LoadoutKitEntry` heap overflow (no callers yet) | `core_base_full_character.h:478` |
| 7 | `endian_read/write_f64` truncation bug | `endian.c:365` |
| 8 | `endian_write_u64_big` parameter type mismatch | `endian.c:545` |
| 9 | `is_wsa_initialized` uninitialized local | `yote_platform.h:173` |
| 10 | CRC never computed (zeros sent) | `coreProtocol.c:50` |
| 11 | `character_stats1_count` always 0 (both branches) | `sendSelfToClient.c:289` |
| 12 | `MAX_PACKET_LENGTH = 512` too small for large packets | `kotk_zone_server.c:35` |
| 13 | `generateRandomGuid` uses unseeded `rand()` | `kotk_zone_server.c:38` |

---

## Key Discoveries

### Protocol 1087 Specifics

- `last_login_date` in `SendSelfToClient` is **u64**, not u32 (u32 breaks `HaveProxiedCharacter`)
- KotK uses **loadout ID 17**, not 3 (Just Survive uses 3)
- KotK base profile is **ID 4** (Unemployed), not 3
- `character_stats1` crashes the 1087 client if populated — always send count=0
- `FullCharacterDataRequest` (`0x0f 0x45`) is **never sent** by the KotK 1087 client — h1emu's invisible character fix via `LightweightToFullPc` does not apply

### Gateway / SOE

- The gateway byte format is `(channel << 5) | packetId`
- `uint2b` encoding: value `1` encodes as `0x04` on the wire (left-shifted by 2 bits)
- Channel 2 carries real-time position updates — previously dropped with an early `return`, now fixed
- `NetworkProximityUpdatesComplete` must be deferred using **wall-clock time** (`__time64_t`), not tick count (tick count is a high-frequency counter making delays instant)
- RC4 encryption key: `\x17\xbd\x08\x6b\x1b\x94\xf0\x2f\xf0\xec\x53\xd7\x63\x58\x9b\x5f`

### Reference Data

- `ReferenceData_DynamicAppearance` (opcode `0x17 0x06`) — hand-built, 3 entries: CharSkinTone (122), CharHairColor (125), CharEyeColor (129)
- All `ZonePacketRawFileSend` calls must use paths relative to where the DLL runs — if paths break, use absolute paths
- `sendself_patched.bin` and `SendSelfToClient-in-route.bin` are **identical** except for 7 header bytes (stream length + guid) — the dynamic packer produces the same content as the captured working binary

### Clothing and Equipment

- Default KotK spawn clothing: `SHIRT_DEFAULT=2088`, `PANTS_DEFAULT=2177` (from `InitializeCharacterDefaultLoadout`)
- Currently equipped: Gas Runner Hoodie (5747), Jeans/SkinnyLeg (2178), Conveys Sneakers (2216)
- Equipment must be sent in `SendEquipmentAndMovement` on `ClientFinishedLoading`, not in `OnLogin` or `DeployCharacter`
- `SetCharacterEquipment` slot 1 (head) must use `guid=0`, not a real guid — using a guid caused the head attachment to fail
- `ActivateProfile` must match `SetCharacterEquipment` attachment list exactly

---

## File Layout

| File | Purpose |
|---|---|
| `src/zone/onLogin.c` | `OnLogin()`, `DeployCharacter()`, `SendEquipmentAndMovement()` |
| `src/zone/sendSelfToClient.c` | `SendSelfToClient()` dynamic packer + `SendSelfToClientRaw()` |
| `src/zone/zonePacketHandler.c` | All incoming zone packet handlers |
| `src/zone/clientProtocol_1087.c` | `ZonePacketSend()`, position parsing, static view |
| `src/zone/gatewayApi.c` | Gateway pack/unpack, tunnel routing, channel routing |
| `src/kotk_zone_server.c` | Main server loop, session init, deferred tick logic |
| `src/kotk_login_server.c` | Login server main loop |
| `src/login/loginPacketHandler.c` | Login packet dispatch |
| `src/login/loginUdp_11.c` | Login send helpers, name validation, character create/select |
| `src/soe/coreProtocol.c` | SOE session/data/ack/fragment handling, 6 channels |
| `src/soe/inputStream.c` | Reliable ordered delivery, RC4 decrypt, fragment assembly |
| `src/soe/outputStream.c` | Reliable send, RC4 encrypt, fragmentation |
| `src/core/entities/core_base_full_character.h` | KotK constants, item enums, loadout kit functions |
| `schema/client_protocol_1087.schm` | Zone protocol schema (auto-generates packers) |
| `data/*.bin` | Raw reference data (Definitions, etc.) |

---

## Remaining TODO (Priority Order)

1. **Invisibility bug** — Identify and send the packet/field that proactively commits the character mesh to the scene graph without requiring the wall trick. Best remaining lead: binary diff `sendself_patched.bin` vs dynamic packer output with verbose field logging to find any discrepancy.

2. **Inventory slot grid** — Determine the correct `contain_def_id` for equipped slot containers. The value should be a fixed "equipped container type" ID the client knows locally, not the item's def_id.

3. **Health bar / HUD resources** — `ResourceEventBase` is sent with 9 resources but the health bar doesn't appear. May require the match state to be initialized first, or a specific resource packet format.

4. **Match state** — Send match initialization packets to trigger the KotK game mode UI (player count, gas timer, team count). This likely requires `POIChangeMessage` with actual data and a match-start signal packet.

5. **`FullCharacterDataRequest` response** — While the KotK client (apparently) doesn't send this, investigate whether `LightweightToFullPc` (0xda) or `LightweightToFullNpc` (0xdb) should be proactively sent by the server to upgrade the lightweight entity. Both packet packers are empty stubs — would require raw binary construction.

6. **World objects** — `AddSimpleNpc` + replication data for Fort Destiny props, vehicles, loot spawns.

7. **Sprint** — Verify `Command_RunSpeed` value and ensure no speed-stacking from duplicate `SendEquipmentAndMovement` calls.

---

## Legal Notes

This project is for educational purposes only. We are not responsible for your actions using it.