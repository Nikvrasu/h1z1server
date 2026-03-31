# Hotbar & Clothing Implementation Guide

## How H1emu/h1z1-server Populates the Hotbar and Character Clothing

This document explains exactly how the reference TypeScript H1emu server
(`GitHub.com/H1emu/h1z1-server`) sends items to the hotbar and clothing to the
character model, and how this C server implements the same logic.

---

## 1. How H1emu Populates the Hotbar

### Data Structures

H1emu uses three interconnected data structures on the server side:

| Structure | TypeScript class / field | Purpose |
|-----------|--------------------------|---------|
| **BaseItem** | `BaseItem` in `classes/baseItem.ts` | Core item instance — has `itemDefinitionId`, `itemGuid`, `stackCount`, `currentDurability` |
| **LoadoutItem** | `LoadoutItem` extends `BaseItem` | Item placed in a hotbar slot — adds `slotId` (hotbar position) and `loadoutItemOwnerGuid` |
| **\_loadout map** | `BaseFullCharacter._loadout` | `{ [slotId]: LoadoutItem }` — maps hotbar slot numbers to `LoadoutItem` instances |

### Flow: Equipping an Item to the Hotbar

1. **Item creation** — `server.generateItem(itemDefinitionId)` produces a new
   `BaseItem` with a unique GUID, durability from the definition table, and
   `stackCount = 1`.

2. **equipItem()** — `character.equipItem(server, item, true, slotId)`:
   - Validates the item definition exists.
   - If `slotId` is 0, auto-resolves to the item's default loadout slot
     (e.g., fists → `LoadoutSlots.FISTS`).
   - Creates a `LoadoutItem(item, slotId, characterId)` wrapper.
   - Stores it: `this._loadout[slotId] = loadoutItem`.
   - Sets `loadoutItem.containerGuid = "0xFFFFFFFFFFFFFFFF"` (sentinel meaning
     "this item lives in the loadout, not in a backpack container").

3. **Packet build** — `pGetInventoryItems()` iterates every entry in
   `_loadout` and serialises each into an `itemData` element containing:

   ```
   itemDefinitionId    // e.g. 85 for fists
   guid                // unique item GUID
   count               // 1
   containerGuid       // 0xFFFFFFFFFFFFFFFF (loadout sentinel)
   containerDefinitionId  // LOADOUT_CONTAINER_ID (type 4)
   containerSlotId     // the loadout slot number
   baseDurability      // from item definition
   currentDurability   // current wear value
   ownerCharacterId    // the player's character ID
   ```

4. **SendSelfToClient** — The array of `itemData` is placed into the
   `itemsData.items` field of the `SendSelfToClient` packet.  Additionally,
   the `loadoutSlots` array maps each hotbar slot to the item GUID:

   ```
   loadoutSlots: [
     { hotbarSlotId: 7, loadoutId: 17, slotId: 7, itemDefId: 85, loadoutItemGuid: <fists GUID> },
     { hotbarSlotId: 5, loadoutId: 17, slotId: 5, itemDefId: 1542, loadoutItemGuid: <binos GUID> },
   ]
   currentSlotId: 7   // fists selected
   ```

5. **Client receives** — The client:
   - Indexes the `items` array by GUID to build its local item database.
   - Reads `loadoutSlots` to map each hotbar position to an item GUID.
   - Looks up the GUID in the item database to get the icon (via
     `itemDefinitionId`) and durability.
   - Renders the hotbar UI.

**Key insight:** Without entries in `items` (items1), the `loadoutSlots` GUIDs
point at nothing, and the hotbar stays blank.

---

## 2. How H1emu Populates Character Clothing

### Data Structures

| Structure | TypeScript class / field | Purpose |
|-----------|--------------------------|---------|
| **CharacterEquipment** | interface in types | Visual slot data — has `modelName`, `slotId`, `guid`, `textureAlias`, `tintAlias` |
| **\_equipment map** | `BaseFullCharacter._equipment` | `{ [equipmentSlotId]: CharacterEquipment }` — maps body slots to 3D models |

### Equipment Slot IDs

| Slot ID | Body Part |
|---------|-----------|
| 3 | Chest (shirt / bra) |
| 4 | Legs (pants / underwear) |
| 7 | Right hand (weapon model) |

### Flow: Equipping Clothing

1. **equipItem()** determines the equipment slot from the item definition's
   `PASSIVE_EQUIP_SLOT_ID` or `ACTIVE_EQUIP_SLOT_ID`.

2. An equipment record is built:
   ```
   {
     modelName:    "SurvivorMale_Chest_Bra.adr"  // from item def MODEL_NAME
     slotId:       3                              // chest slot
     guid:         <item GUID>                    // matches the BaseItem
     textureAlias: "Default"
     tintAlias:    "Default"
   }
   ```
   Stored in `this._equipment[3]`.

3. **SendSelfToClient** includes the `equipmentSlots` array (from
   `pGetEquipmentSlots()`) and the `attachmentData` array (from
   `pGetAttachmentSlots()`):

   - **equipmentSlots** — slot ID + item GUID + tint/decal, tells the client
     *which item occupies each body slot*.
   - **attachmentData** — model name + texture + slot ID, tells the client
     *what 3D model to render in each slot*.

4. **Equipment.SetCharacterEquipment** — h1emu also sends a separate
   `SetCharacterEquipment` packet during `ClientFinishedLoading` to reinforce
   the visual state.

5. **Client receives** — reads equipment slots for GUIDs (cross-references
   with item data), then reads attachment data for the `.adr` model file to
   load.

---

## 3. Implementation Plan for the C Server

### Problem

The C server already had:
- ✅ `loadout_slots_array` with fists + binoculars (correct GUIDs)
- ✅ `equipment_slots` with chest, legs, and fists models
- ✅ `Equipment.SetCharacterEquipment` sent in `ClientFinishedLoading`
- ❌ `items1_count = 0` — **no item data** backing the GUIDs

The hotbar was blank because `items1` was empty.  The client could not resolve
the GUIDs from `loadout_slots_array` to actual items.

### Changes Made

#### A. New constants (`core_base_full_character.h`)

```c
#define ITEM_GUID_CHEST_CLOTHING  0x0001000000000003ULL
#define ITEM_GUID_LEGS_CLOTHING   0x0001000000000004ULL
```

Previously equipment slots for chest and legs used ad-hoc values `0x1001` /
`0x1002`.  Named constants keep all GUID references consistent.

#### B. Populated `items1` array (`sendSelfToClient.c`)

Four items added to `items1`:

| Index | Item | item\_def\_id | GUID constant | container\_slot\_id |
|-------|------|---------------|---------------|---------------------|
| 0 | Fists | 85 (`WEAPON_FISTS`) | `ITEM_GUID_FISTS` | 7 (LOADOUT\_SLOT\_MELEE) |
| 1 | Binoculars | 1542 (`WEAPON_BINOCULARS`) | `ITEM_GUID_BINOCULARS` | 5 (LOADOUT\_SLOT\_BINOCULARS) |
| 2 | Chest bra | 2088 (`SHIRT_DEFAULT`) | `ITEM_GUID_CHEST_CLOTHING` | 3 |
| 3 | Underwear | 2177 (`PANTS_DEFAULT`) | `ITEM_GUID_LEGS_CLOTHING` | 4 |

Each item has:
- `container_guid = 0xFFFFFFFFFFFFFFFF` — h1emu's loadout sentinel
- `container_def_id = 4` — loadout container type
- `owner_character_id = session->characterId`
- Appropriate durability values

#### C. Updated equipment slot GUIDs

`equipment_slots` in `sendSelfToClient.c` and the
`Equipment.SetCharacterEquipment` packet in `onLogin.c` now reference
`ITEM_GUID_CHEST_CLOTHING` / `ITEM_GUID_LEGS_CLOTHING` instead of the old
hard-coded `0x1001` / `0x1002`.

---

## 4. Why This Works

### The Three-Pillar Model

The H1Z1 client requires **three data pillars** to display a character:

```
               SendSelfToClient
              /        |        \
       items1     loadout_slots   equipment_slots
     (item DB)    (hotbar map)    (3D visuals)
```

1. **items1** is the item database.  Every item the client can interact with
   must exist here, with a unique GUID, definition ID, durability, and owner.

2. **loadout\_slots\_array** maps each hotbar position to an item GUID.  The
   client looks up the GUID in items1 to get the icon and metadata.

3. **equipment\_slots** maps each body slot to a 3D model file (`.adr`) and a
   GUID.  The model file tells the renderer what mesh to draw; the GUID
   cross-references the item for tint / durability overlay.

**If any pillar is missing, the feature breaks:**

| Missing | Symptom |
|---------|---------|
| items1 empty | Hotbar slots show blank — no icon or durability |
| loadout\_slots empty | Hotbar has no slots at all |
| equipment\_slots empty | Character appears naked / no weapon model |

### GUID Consistency

Every GUID that appears in `loadout_slots_array` or `equipment_slots` **must**
also appear in `items1`.  The client treats `items1` as the authoritative
registry; all other arrays merely reference items by GUID.

By adding the four items to `items1` with matching GUIDs, the client can now:
1. Build its item database from `items1`.
2. Resolve `loadout_slots_array` → GUID → item → icon → display in hotbar.
3. Resolve `equipment_slots` → GUID → item → tint/durability → render model.

### Packet Sequence

```
OnLogin()
  ├── InitializationParameters
  ├── SendZoneDetails
  ├── ClientGameSettings
  ├── ReferenceData.DynamicAppearance
  ├── SendSelfToClient          ◄── items1 + loadout + equipment all here
  ├── Container.InitEquippedContainers
  ├── Command_ItemDefinitions    ◄── client needs defs to decode item_def_id
  ├── ReferenceData_WeaponDefinitions
  └── ClientBeginZoning

ClientIsReady  →  DeployCharacter()
  ├── UpdateCharacterState
  ├── DoneSendingPreloadCharacters
  ├── CharacterStateDelta
  └── ZoneDoneSendingInitialData

ClientFinishedLoading  →  SendEquipmentAndMovement()
  ├── UpdateWeatherData
  ├── Character.WeaponStance
  ├── Equipment.SetCharacterEquipment  ◄── reinforces clothing visuals
  └── Command.RunSpeed / ModifyMovementSpeed
```

The key moment is `SendSelfToClient`: this is the **only** packet that
delivers the items1 array, loadout slots, and initial equipment slots to the
client in one shot.  `SetCharacterEquipment` later reinforces the visual
state but cannot introduce new items — it only references GUIDs the client
already knows about from items1.
