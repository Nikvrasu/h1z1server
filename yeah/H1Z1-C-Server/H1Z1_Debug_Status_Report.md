# H1Z1 KotK C Server — Character Rendering & Speed Debug Status

## Project
C server emulator for H1Z1: King of the Kill, Preseason 3, protocol 1087.
Path: `D:\h1z1server\yeah\H1Z1-C-Server\`

---

## What Works (100% confirmed)

- **Login flow**: Login server → zone server handoff works
- **Zone loading**: Client loads Z2 map, terrain renders, sky/weather works
- **Camera**: Attached to character position, can look around with mouse
- **Movement input**: WASD moves the character, jumping works
- **Packet pipeline**: SOE protocol with RC4 encryption, multi-channel reliable UDP, fragment reassembly — all working
- **Schema tool**: Generates C packers/unpackers from `.schm` file, packs SendSelfToClient correctly
- **Two-phase login flow**: Phase 1 (OnLogin) → Phase 2 (DeployCharacter on ClientIsReady) → Phase 3 (ClientFinishedLoading)
- **PlayerWorldTransferRequest**: Lobby → game transition works
- **actorModelId = 9469**: Confirmed correct for male survivor (matches h1emu and reference binary)
- **Packet format**: `u64 last_login_date` is correct for protocol 1087 (changing to u32 breaks everything)
- **KeepAlive**: Client sends keepalives, server responds
- **0x1144 values**: With actorModelId=9469, client reports normal-looking values (~1.67, 1.42, 1.26) instead of the old constant 0.58

## What Doesn't Work (the 3 problems)

### 1. Character Model Invisible (PARTIALLY FIXED — intermittent)
- First person: no hands/arms visible
- Third person: character appeared ONCE — naked male with underpants, correct model
- The model appeared briefly then disappeared when pressing T (third-person toggle)
- Visibility seems RANDOM — same code sometimes shows the model, sometimes doesn't
- This points to a **timing/race condition**, not a data problem

### 2. Movement Is Too Slow
- Feels like crouch-walking speed
- Cannot sprint (Shift+W does nothing different)
- The slowness persists regardless of all fixes attempted
- The 0x1144 packet values changed from 0.58 (with wrong actorModelId) to ~1.67 (with correct actorModelId), suggesting the client's internal speed DID change, but it still feels slow

### 3. Inventory Empty
- No fists, no binoculars in hotbar
- Red empty squares visible in hotbar UI
- Loadout and equipment packets are being sent but items don't appear

---

## What We Tried — Detailed List

### For Character Visibility:

| # | What we tried | Result |
|---|---------------|--------|
| 1 | Changed actorModelId from 9240 to 9469 | No visible change (but 0x1144 values improved) |
| 2 | Added Character.WeaponStance { stance: 1 } in DeployCharacter | No visible change |
| 3 | Moved WeaponStance to ClientFinishedLoading | Model appeared ONCE then became intermittent |
| 4 | Set WeaponStance stance = 0 (unarmed) | Made character invisible |
| 5 | Removed AddLightweightPc from DeployCharacter | No change (neither helped nor broke) |
| 6 | Added AddLightweightPc back in DeployCharacter | Model appeared once |
| 7 | Moved AddLightweightPc to ClientFinishedLoading | Intermittent — same random behavior |
| 8 | Sent Equipment.SetCharacterEquipment in ClientFinishedLoading (h1emu says "needed or 3rd person invisible") | No consistent change |
| 9 | Populated equipment_slots with fists at slot 7 (Weapon_Empty.adr) | No consistent change |
| 10 | Removed Respawn + RespawnReply packets | Model appeared once after this |
| 11 | Set trigger_loading_screen = FALSE on UpdateLocation in Deploy | No clear change |
| 12 | Set head_id=5, unk_u32_3=5, faction_id=662 (matching reference binary) | No change |

### For Movement Speed:

| # | What we tried | Result |
|---|---------------|--------|
| 1 | Changed actorModelId from 9240 to 9469 | 0x1144 values changed from 0.58 to ~1.67 but subjectively still slow |
| 2 | Sent Command.RunSpeed { runSpeed: 0 } in ClientFinishedLoading | No change |
| 3 | Built and sent ReferenceData.ProfileDefinitions (0x1703) with 7 profiles, profile 5 having unknownFloat1=1.7 | No change |
| 4 | Added 32 character_stats1 entries (movement stats from h1emu's stats.json) | CRASHES the protocol 1087 client |
| 5 | Sent all reference data .bin files (ItemDefinitions, WeaponDefinitions, ProjectileDefinitions, ItemClassDefinitions, ProfileDefinitions) | No change |
| 6 | Sent Command.ItemDefinitions and ReferenceData.WeaponDefinitions in OnLogin before SendSelfToClient | No change |

### For Packet Format:

| # | What we tried | Result |
|---|---------------|--------|
| 1 | Changed last_login_date from u64 to u32 in schema | BROKE EVERYTHING — infinite loading, HaveProxiedCharacter=0 |
| 2 | Reverted back to u64 | Restored working state |
| 3 | Byte-by-byte comparison with Z1BR reference binary | Confirmed reference uses different protocol version — cannot compare directly |

---

## What We Know 100% For Sure It ISN'T

1. **NOT a wrong actorModelId** — 9469 is confirmed correct, and the model DID render once
2. **NOT a packet format/alignment issue in SendSelfToClient** — the client parses it correctly (gets position, name, loads into game)
3. **NOT missing reference data** — we send ItemDefinitions, WeaponDefinitions, ProjectileDefinitions, ItemClassDefinitions, ProfileDefinitions
4. **NOT a wrong schema for last_login_date** — u64 is correct for protocol 1087
5. **NOT character_stats1** — adding stats crashes the 1087 client; the KotK client doesn't want them in SendSelfToClient
6. **NOT Command.RunSpeed** — sending { runSpeed: 0 } doesn't change anything
7. **NOT a fundamentally broken pipeline** — the character model DID appear once (naked male in underpants, correct third-person rendering)

---

## Current Source-of-Truth Sequence

- `OnLogin`: `InitializationParameters` → `SendZoneDetails` → `ClientGameSettings` → `ReferenceData.DynamicAppearance` → `SendSelfToClient` → `Container.InitEquippedContainers` → raw reference blobs → `ClientBeginZoning`
- `DeployCharacter` on `ClientIsReady`: `POIChangeMessage` → `Character.UpdateCharacterState` → `ClientUpdate.DoneSendingPreloadCharacters` → `DtoObjectInitialData` → `Character.CharacterStateDelta` → `ZoneDoneSendingInitialData` → deferred `ClientUpdate.NetworkProximityUpdatesComplete`
- `SendEquipmentAndMovement` on `ClientFinishedLoading`: `UpdateWeatherData` → `Character.WeaponStance` → `Equipment.SetCharacterEquipment` → `Command.RunSpeed` → `ClientUpdate.ModifyMovementSpeed`

Implementation notes:

- `DtoObjectInitialData` is now treated as opcode `0xbb0300` with the minimal H1emu-shaped payload.
- `ReferenceData.DynamicAppearance` now has a runtime fallback packet if the cached binary file is missing.
- Login/deploy now trace packet order with queued send sequence numbers to make race conditions visible in logs.

## What We Suspect It MIGHT Be

### For Visibility (most likely → least likely):

1. **Race condition / packet ordering** — The model appeared once and then inconsistently. All packets arrive in one burst within a single tick. The client may process them in unpredictable order. `NetworkProximityUpdatesComplete` is currently deferred, but the `ClientIsReady` burst still needs trace verification around `UpdateCharacterState` → `DoneSendingPreloadCharacters` → `DtoObjectInitialData` → `CharacterStateDelta`.

2. **Missing or wrong packet that kills the entity after creation** — Something in our flow might be destroying/hiding the entity shortly after AddLightweightPc creates it. The Respawn packets were suspected (removing them coincided with the model appearing once, but couldn't reproduce).

3. **The character needs to be in a specific "alive" state** — There might be a Character.UpdateCharacterState or similar packet that marks the character as alive/renderable. Without it, the client may garbage-collect the entity after a frame or two.

4. **SendSelfToClient alone should render the self-character (no AddLightweightPc needed)** — h1emu explicitly says AddLightweightPc is for OTHER players, not self. But without it, we never see anything. The self-rendering from SendSelfToClient might require specific field values we're not setting (is_respawning, some unknown byte, etc.).

### For Speed:

1. **The loadout/weapon stance is throttling movement** — The character appeared to be "holding something" (like a knife animation). Weapon-drawn state reduces movement speed in H1Z1. Without proper fists initialization, the character might be in a bugged weapon state.

2. **Profile definitions aren't being parsed correctly** — The ProfileDefinitions binary we built manually might have wrong field ordering for protocol 1087 (we guessed the format from h1emu's TypeScript schema).

3. **Sprint requires stamina resource to be properly initialized** — We send stamina=10000 but maybe the sprint system needs additional flags or a different resource type.

4. **The base speed IS correct and what we're seeing is walk speed** — The ~1.67 value matches the 1.7 profile run speed. Sprint might need a completely different activation mechanism in KotK PS3.

---

## What We Haven't Tried Yet

1. **Adding a deliberate delay** between packets (especially before AddLightweightPc and after ZoneDoneSendingInitialData) to avoid race conditions

2. **Sending ClientUpdate.ActivateProfile** — commented out in h1emu but might be needed for protocol 1087. Contains actorModelId, profileId, tintAlias, decalAlias

3. **Sending Loadout.SetCurrentLoadout** — also commented out in h1emu

4. **Capturing packets from a working h1emu server** (the main `QuentinGruber/h1z1-server` repo, not the abandoned kotk-server) and comparing them against ours

5. **Setting `is_respawning = TRUE`** in SendSelfToClient (instead of FALSE) — might affect character state initialization

6. **Trying different values for `unk_byte_2`, `unk_byte_31`, `unk_byte_4`, `unk_byte_5`** in SendSelfToClient — these unknown bytes between equipment and stats might control rendering flags

7. **Sending the character as an NPC first** (AddSimpleNpc or similar) to test if the rendering issue is specific to the PC entity type

8. **Dumping the client's internal state** via the dev console (F1) to see what the client thinks the character state is

9. **Testing with a female character** (actorModelId=9474) to rule out male-specific model issues

10. **Sending Equipment.SetCharacterEquipment with 0 equipment slots** (completely empty, no fists) to see if the equipment data is confusing the renderer

11. **Investigating the `shaderGroupId` field** mentioned by Copilot — might need to be set to 122 for Head_01

---

## Current Code State

- **DeployCharacter**: Sends SendSelfToClient → UpdateLocation(trigger=FALSE) → ContainerInit → CharacterStateDelta → GameTimeSync → ReferenceData files → DoneSendingPreloadCharacters → NetworkProximityUpdatesComplete → ZoneDoneSendingInitialData

- **ClientFinishedLoading**: Sends AddLightweightPc → Equipment (fists) → Loadout (fists+binoculars) → WeaponStance(stance=1) → RunSpeed(0)

- **AddLightweightPc and Equipment/Loadout are currently in ClientFinishedLoading**, removed from DeployCharacter

- **Respawn + RespawnReply are currently commented out**

- **character_stats1 is disabled** (count = 0 regardless of withStats flag)
