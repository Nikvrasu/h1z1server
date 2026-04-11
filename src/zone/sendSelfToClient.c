u32 GetActorModelId(SessionState* session) {
    u32 headId = session->pGetPlayerActor.headType;

    switch (headId) {
        case 1: {
            session->pGetPlayerActor.actorModelId = 9469;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 2: {
            session->pGetPlayerActor.actorModelId = 9469;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 3: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 4: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 5: {
            session->pGetPlayerActor.actorModelId = 9469;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 6: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 7: {
            session->pGetPlayerActor.actorModelId = 9469;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 8: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        default: {
            return 0;
        }
    }
}

u32 GetGender(SessionState* session) {
    u32 actorModelId = session->pGetPlayerActor.actorModelId;

    switch (actorModelId) {
        case 9469: {
            session->pGetPlayerActor.gender = 1;
            return session->pGetPlayerActor.gender;
        } break;
        case 9474: {
            session->pGetPlayerActor.gender = 2;
            return session->pGetPlayerActor.gender;
        } break;
        default: {
            return 0;
        }
    }
}

char* GetHairModel(u32 actorModelId) {
    switch (actorModelId) {
        case 9469:
            printf("Male Hair Model Selected!\n");
            return "SurvivorMale_Hair_MediumMessy.adr";
        case 9474:
            printf("Female Hair Model Selected!\n");
            return "SurvivorFemale_Hair_ShortBun.adr";
        default:
            return "";
    }
}

u32 getResourceType(u32 resourceId) {
    switch (resourceId) {
        case HEALTHID:
            return HEALTHTYPE;
        case HUNGERID:
            return HUNGERTYPE;
        case HYDRATIONID:
            return HYDRATIONTYPE;
        case STAMINAID:
            return STAMINATYPE;
        case VIRUSID:
            return VIRUSTYPE;
        case BLEEDINGID:
            return BLEEDINGTYPE;
        case COMFORTID:
            return COMFORTTYPE;
        case FUELID:
            return FUELTYPE;
        case CONDITIONID:
            return CONDITIONTYPE;
        default:
            return 0;
    }
}

#define CHARACTER_INIT_MAX_ITEMS 5
#define CHARACTER_INIT_MAX_EQUIPMENT_SLOTS 7
#define CHARACTER_INIT_MAX_LOADOUT_SLOTS 2
#define CHARACTER_INIT_MAX_ATTACHMENTS 7
#define CHARACTER_INIT_STATE_CACHE_SIZE 64

typedef struct CharacterInitItem {
    u32 item_def_id;
    u64 guid;
    u32 loadout_slot_id;
    u32 equipment_slot_id;
    u64 container_guid;
    u32 container_def_id;
    u32 container_slot_id;
    u64 owner_character_id;
} CharacterInitItem;

typedef struct CharacterInitLoadoutSlot {
    u32 hotbar_slot_id;
    u32 slot_id;
    u32 item_def_id;
    u64 loadout_item_guid;
} CharacterInitLoadoutSlot;

typedef struct CharacterInitEquipmentSlot {
    u32 equipment_slot_id;
    u64 guid;
} CharacterInitEquipmentSlot;

typedef struct CharacterInitAttachment {
    u32 slot_id;
    String8 model_name;
} CharacterInitAttachment;

typedef struct CharacterInitState {
    u64 character_id;
    u32 transient_id;
    u32 actor_model_id;
    u32 gender;
    u32 head_type;
    String8 character_name;
    String8 head_actor;
    String8 hair_model;
    vec4 position;
    vec4 rotation;
    u32 profile_id;
    u32 loadout_id;
    u32 current_loadout_slot_id;

    u32 items_count;
    CharacterInitItem items[CHARACTER_INIT_MAX_ITEMS];

    u32 loadout_slots_count;
    CharacterInitLoadoutSlot loadout_slots[CHARACTER_INIT_MAX_LOADOUT_SLOTS];

    u32 equipment_slots_count;
    CharacterInitEquipmentSlot equipment_slots[CHARACTER_INIT_MAX_EQUIPMENT_SLOTS];

    u32 attachments_count;
    CharacterInitAttachment attachments[CHARACTER_INIT_MAX_ATTACHMENTS];

    u8 dto_payload[14];
    u32 dto_payload_len;
} CharacterInitState;

typedef struct CharacterInitStateCacheEntry {
    SessionState* session;
    u32 zone_cycle_id;
    b8 valid;
    CharacterInitState state;
} CharacterInitStateCacheEntry;

static CharacterInitStateCacheEntry g_characterInitStateCache[CHARACTER_INIT_STATE_CACHE_SIZE] = { 0 };

static String8 CharacterInitFindAttachmentModel(const CharacterInitState* state, u32 slotId) {
    for (u32 i = 0; i < state->attachments_count; i++) {
        if (state->attachments[i].slot_id == slotId) {
            return state->attachments[i].model_name;
        }
    }
    return STR8("");
}

static b8 CharacterInitBuildFromSession(SessionState* session, CharacterInitState* outState) {
    if (!session || !outState) return FALSE;

    memset(outState, 0, sizeof(*outState));

    u32 actorModelId = session->pGetPlayerActor.actorModelId;
    if (actorModelId == 0) actorModelId = 9469;

    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = (actorModelId == 9474) ? 2 : 1;

    u32 headType = session->pGetPlayerActor.headType;
    if (headType == 0) headType = (gender == 2) ? 3 : 1;

    String8 headActor = session->pGetPlayerActor.headActor;
    if (headActor.size == 0) {
        headActor = (gender == 2) ? STR8("SurvivorFemale_Head_01.adr") : STR8("SurvivorMale_Head_01.adr");
    }

    String8 hairModel = session->pGetPlayerActor.hairModel;
    if (hairModel.size == 0) {
        hairModel = (gender == 2) ? STR8("SurvivorFemale_Hair_ShortBun.adr") : STR8("SurvivorMale_Hair_MediumMessy.adr");
    }

    String8 chestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 legsModel = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
    String8 hoodieModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Hoodie_Down.adr") : STR8("SurvivorMale_Chest_Hoodie_Down.adr");
    String8 pantsModel = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_SkinnyLeg.adr") : STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr");
    String8 shoesModel = (gender == 2) ? STR8("SurvivorFemale_Feet_Conveys.adr") : STR8("SurvivorMale_Feet_Conveys.adr");

    outState->character_id = session->characterId;
    outState->transient_id = session->zoneCycleId ? session->zoneCycleId : 1;
    outState->actor_model_id = actorModelId;
    outState->gender = gender;
    outState->head_type = headType;
    outState->character_name = (session->characterName.size > 0) ? session->characterName : STR8("Unknown");
    outState->head_actor = headActor;
    outState->hair_model = hairModel;
    outState->position = (vec4){ .x = -297.309998f, .y = 506.059998f, .z = -4894.100098f, .w = 1.0f };
    outState->rotation = (vec4){ .x = 0.0f, .y = -0.707100f, .z = 0.0f, .w = 0.707100f };
    outState->profile_id = KOTK_CHARACTER_PROFILE_ID;
    outState->loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    outState->current_loadout_slot_id = LOADOUT_SLOT_MELEE;

    outState->items_count = CHARACTER_INIT_MAX_ITEMS;
    outState->items[0] = (CharacterInitItem){
        .item_def_id = WEAPON_FISTS,
        .guid = ITEM_GUID_FISTS,
        .loadout_slot_id = LOADOUT_SLOT_MELEE,
        .equipment_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
        .container_guid = LOADOUT_CONTAINER_GUID,
        .container_def_id = LOADOUT_CONTAINER_ID,
        .container_slot_id = LOADOUT_SLOT_MELEE,
        .owner_character_id = session->characterId,
    };

    outState->items[1] = (CharacterInitItem){
        .item_def_id = WEAPON_BINOCULARS,
        .guid = ITEM_GUID_BINOCULARS,
        .loadout_slot_id = LOADOUT_SLOT_BINOCULARS,
        .equipment_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
        .container_guid = LOADOUT_CONTAINER_GUID,
        .container_def_id = LOADOUT_CONTAINER_ID,
        .container_slot_id = LOADOUT_SLOT_BINOCULARS,
        .owner_character_id = session->characterId,
    };
    outState->items[2] = (CharacterInitItem){
        .item_def_id = ITEM_DEF_HOODIE,
        .guid = ITEM_GUID_HOODIE,
        .loadout_slot_id = LOADOUT_SLOT_CHEST,
        .equipment_slot_id = 10,
        .container_guid = ITEM_GUID_HOODIE,
        .container_def_id = 1,
        .container_slot_id = LOADOUT_SLOT_CHEST,
        .owner_character_id = session->characterId,
    };
    outState->items[3] = (CharacterInitItem){
        .item_def_id = ITEM_DEF_SKINNY_JEANS,
        .guid = ITEM_GUID_JEANS,
        .loadout_slot_id = LOADOUT_SLOT_LEGS,
        .equipment_slot_id = LOADOUT_SLOT_LEGS,
        .container_guid = ITEM_GUID_JEANS,
        .container_def_id = 1,
        .container_slot_id = LOADOUT_SLOT_LEGS,
        .owner_character_id = session->characterId,
    };
    outState->items[4] = (CharacterInitItem){
        .item_def_id = ITEM_DEF_CONVEYS,
        .guid = ITEM_GUID_SHOES,
        .loadout_slot_id = LOADOUT_SLOT_FEET,
        .equipment_slot_id = LOADOUT_SLOT_FEET,
        .container_guid = ITEM_GUID_SHOES,
        .container_def_id = 1,
        .container_slot_id = LOADOUT_SLOT_FEET,
        .owner_character_id = session->characterId,
    };

    outState->loadout_slots_count = CHARACTER_INIT_MAX_LOADOUT_SLOTS;
    outState->loadout_slots[0] = (CharacterInitLoadoutSlot){
        .hotbar_slot_id = LOADOUT_SLOT_MELEE,
        .slot_id = LOADOUT_SLOT_MELEE,
        .item_def_id = WEAPON_FISTS,
        .loadout_item_guid = ITEM_GUID_FISTS,
    };
    outState->loadout_slots[1] = (CharacterInitLoadoutSlot){
        .hotbar_slot_id = LOADOUT_SLOT_BINOCULARS,
        .slot_id = LOADOUT_SLOT_BINOCULARS,
        .item_def_id = WEAPON_BINOCULARS,
        .loadout_item_guid = ITEM_GUID_BINOCULARS,
    };

    outState->equipment_slots_count = CHARACTER_INIT_MAX_EQUIPMENT_SLOTS;
    outState->equipment_slots[0] = (CharacterInitEquipmentSlot){ .equipment_slot_id = EQUIPMENT_SLOT_HEAD, .guid = 0 };
    outState->equipment_slots[1] = (CharacterInitEquipmentSlot){ .equipment_slot_id = EQUIPMENT_SLOT_CHEST, .guid = 0 };
    outState->equipment_slots[2] = (CharacterInitEquipmentSlot){ .equipment_slot_id = EQUIPMENT_SLOT_LEGS, .guid = 0 };
    outState->equipment_slots[3] = (CharacterInitEquipmentSlot){ .equipment_slot_id = EQUIPMENT_SLOT_RIGHT_HAND, .guid = ITEM_GUID_FISTS };
    outState->equipment_slots[4] = (CharacterInitEquipmentSlot){ .equipment_slot_id = 10, .guid = ITEM_GUID_HOODIE };
    outState->equipment_slots[5] = (CharacterInitEquipmentSlot){ .equipment_slot_id = LOADOUT_SLOT_LEGS, .guid = ITEM_GUID_JEANS };
    outState->equipment_slots[6] = (CharacterInitEquipmentSlot){ .equipment_slot_id = LOADOUT_SLOT_FEET, .guid = ITEM_GUID_SHOES };

    outState->attachments_count = CHARACTER_INIT_MAX_ATTACHMENTS;
    outState->attachments[0] = (CharacterInitAttachment){ .slot_id = EQUIPMENT_SLOT_HEAD, .model_name = headActor };
    outState->attachments[1] = (CharacterInitAttachment){ .slot_id = EQUIPMENT_SLOT_CHEST, .model_name = chestModel };
    outState->attachments[2] = (CharacterInitAttachment){ .slot_id = EQUIPMENT_SLOT_LEGS, .model_name = legsModel };
    outState->attachments[3] = (CharacterInitAttachment){ .slot_id = EQUIPMENT_SLOT_RIGHT_HAND, .model_name = STR8("Weapon_Empty.adr") };
    outState->attachments[4] = (CharacterInitAttachment){ .slot_id = 10, .model_name = hoodieModel };
    outState->attachments[5] = (CharacterInitAttachment){ .slot_id = LOADOUT_SLOT_LEGS, .model_name = pantsModel };
    outState->attachments[6] = (CharacterInitAttachment){ .slot_id = LOADOUT_SLOT_FEET, .model_name = shoesModel };

    outState->dto_payload_len = 14;
    {
        u8 dtoPayload[] = {
            0x05, 0x03,
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        memcpy(outState->dto_payload, dtoPayload, sizeof(dtoPayload));
    }

    return TRUE;
}

static i32 CharacterInitFindItemIndexByGuid(const CharacterInitState* state, u64 guid) {
    for (u32 i = 0; i < state->items_count; i++) {
        if (state->items[i].guid == guid) return (i32)i;
    }
    return -1;
}

static b8 ValidateCharacterInitState(SessionState* session, const CharacterInitState* state) {
    if (!session || !state) {
        printf("[CHAR_INIT] ValidateCharacterInitState failed: null input\n");
        return FALSE;
    }

    b8 valid = TRUE;

    for (u32 i = 0; i < state->items_count; i++) {
        const CharacterInitItem* item = &state->items[i];

        if (item->owner_character_id != session->characterId) {
            printf("[CHAR_INIT] owner mismatch guid=0x%llx owner=0x%llx expected=0x%llx\n",
                   (unsigned long long)item->guid,
                   (unsigned long long)item->owner_character_id,
                   (unsigned long long)session->characterId);
            valid = FALSE;
        }

        for (u32 j = i + 1; j < state->items_count; j++) {
            if (item->guid == state->items[j].guid) {
                printf("[CHAR_INIT] duplicate item guid=0x%llx\n",
                       (unsigned long long)item->guid);
                valid = FALSE;
            }
        }
    }

    for (u32 i = 0; i < state->loadout_slots_count; i++) {
        const CharacterInitLoadoutSlot* slot = &state->loadout_slots[i];
        i32 itemIndex = CharacterInitFindItemIndexByGuid(state, slot->loadout_item_guid);
        if (itemIndex < 0) {
            printf("[CHAR_INIT] loadout guid missing in items1 guid=0x%llx slot=%u\n",
                   (unsigned long long)slot->loadout_item_guid, slot->slot_id);
            valid = FALSE;
            continue;
        }

        if (state->items[itemIndex].item_def_id != slot->item_def_id) {
            printf("[CHAR_INIT] loadout item_def mismatch slot=%u expected=%u got=%u\n",
                   slot->slot_id, slot->item_def_id, state->items[itemIndex].item_def_id);
            valid = FALSE;
        }
    }

    for (u32 i = 0; i < state->equipment_slots_count; i++) {
        const CharacterInitEquipmentSlot* slot = &state->equipment_slots[i];
        if (slot->guid == 0) continue;

        if (CharacterInitFindItemIndexByGuid(state, slot->guid) < 0) {
            printf("[CHAR_INIT] equipment guid missing in items1 guid=0x%llx slot=%u\n",
                   (unsigned long long)slot->guid, slot->equipment_slot_id);
            valid = FALSE;
        }
    }

    if (state->loadout_slots_count >= 2) {
        const CharacterInitLoadoutSlot* melee = &state->loadout_slots[0];
        const CharacterInitLoadoutSlot* bino = &state->loadout_slots[1];
        if (!(melee->slot_id == LOADOUT_SLOT_MELEE
              && melee->item_def_id == WEAPON_FISTS
              && melee->loadout_item_guid == ITEM_GUID_FISTS)) {
            printf("[CHAR_INIT] melee loadout invariant failed\n");
            valid = FALSE;
        }
        if (!(bino->slot_id == LOADOUT_SLOT_BINOCULARS
              && bino->item_def_id == WEAPON_BINOCULARS
              && bino->loadout_item_guid == ITEM_GUID_BINOCULARS)) {
            printf("[CHAR_INIT] binoculars loadout invariant failed\n");
            valid = FALSE;
        }
    } else {
        printf("[CHAR_INIT] loadout slot count invariant failed\n");
        valid = FALSE;
    }

    i32 fistsItem = CharacterInitFindItemIndexByGuid(state, ITEM_GUID_FISTS);
    i32 binoItem = CharacterInitFindItemIndexByGuid(state, ITEM_GUID_BINOCULARS);
    i32 hoodieItem = CharacterInitFindItemIndexByGuid(state, ITEM_GUID_HOODIE);
    i32 jeansItem = CharacterInitFindItemIndexByGuid(state, ITEM_GUID_JEANS);
    i32 shoesItem = CharacterInitFindItemIndexByGuid(state, ITEM_GUID_SHOES);
    if (fistsItem < 0 || binoItem < 0) {
        printf("[CHAR_INIT] required fists/binocular items missing\n");
        valid = FALSE;
    } else {
        if (!(state->items[fistsItem].container_guid == LOADOUT_CONTAINER_GUID
              && state->items[fistsItem].container_def_id == LOADOUT_CONTAINER_ID)) {
            printf("[CHAR_INIT] fists container invariant failed\n");
            valid = FALSE;
        }
        if (!(state->items[binoItem].container_guid == LOADOUT_CONTAINER_GUID
              && state->items[binoItem].container_def_id == LOADOUT_CONTAINER_ID)) {
            printf("[CHAR_INIT] binoculars container invariant failed\n");
            valid = FALSE;
        }
    }

    if (hoodieItem < 0 || jeansItem < 0 || shoesItem < 0) {
        printf("[CHAR_INIT] clothing guid-backed starter item missing\n");
        valid = FALSE;
    } else {
        if (state->items[hoodieItem].item_def_id != ITEM_DEF_HOODIE
            || state->items[hoodieItem].container_slot_id != LOADOUT_SLOT_CHEST) {
            printf("[CHAR_INIT] hoodie slot/item invariant failed\n");
            valid = FALSE;
        }
        if (state->items[jeansItem].item_def_id != ITEM_DEF_SKINNY_JEANS
            || state->items[jeansItem].container_slot_id != LOADOUT_SLOT_LEGS) {
            printf("[CHAR_INIT] jeans slot/item invariant failed\n");
            valid = FALSE;
        }
        if (state->items[shoesItem].item_def_id != ITEM_DEF_CONVEYS
            || state->items[shoesItem].container_slot_id != LOADOUT_SLOT_FEET) {
            printf("[CHAR_INIT] shoes slot/item invariant failed\n");
            valid = FALSE;
        }
    }

    if (state->profile_id != KOTK_CHARACTER_PROFILE_ID || state->loadout_id != LOADOUT_ID_KOTK_CHARACTER) {
        printf("[CHAR_INIT] profile/loadout invariant failed profile=%u loadout=%u\n",
               state->profile_id, state->loadout_id);
        valid = FALSE;
    }

    if (!valid) {
        printf("[CHAR_INIT] ValidateCharacterInitState hard-failed for charId=0x%llx cycle=%u\n",
               (unsigned long long)session->characterId, session->zoneCycleId);
    }

    return valid;
}

static b8 GetCharacterInitState(SessionState* session, CharacterInitState** outState) {
    if (!session || !outState) return FALSE;

    u32 cycle = session->zoneCycleId ? session->zoneCycleId : 1;
    CharacterInitStateCacheEntry* freeEntry = 0;
    CharacterInitStateCacheEntry* entry = 0;

    for (u32 i = 0; i < CHARACTER_INIT_STATE_CACHE_SIZE; i++) {
        if (g_characterInitStateCache[i].session == session) {
            entry = &g_characterInitStateCache[i];
            break;
        }
        if (!freeEntry && g_characterInitStateCache[i].session == 0) {
            freeEntry = &g_characterInitStateCache[i];
        }
    }

    if (!entry) {
        entry = freeEntry ? freeEntry : &g_characterInitStateCache[0];
        memset(entry, 0, sizeof(*entry));
        entry->session = session;
    }

    if (!entry->valid || entry->zone_cycle_id != cycle) {
        entry->zone_cycle_id = cycle;
        entry->valid = CharacterInitBuildFromSession(session, &entry->state)
                       && ValidateCharacterInitState(session, &entry->state);
        if (!entry->valid) return FALSE;
    }

    *outState = &entry->state;
    return TRUE;
}

static void CharacterInitDebugTraceMismatch(const char* packet, const char* message) {
#if defined(YOTE_INTERNAL)
    printf("[CHAR_INIT TRACE] %s mismatch: %s\n", packet, message);
#else
    UNUSED(packet);
    UNUSED(message);
#endif
}

static void CharacterInitTraceProfileLoadout(const char* packet, const CharacterInitState* state,
                                             u32 profileId, u32 loadoutId) {
#if defined(YOTE_INTERNAL)
    if (profileId != state->profile_id) {
        CharacterInitDebugTraceMismatch(packet, "profile_id");
    }
    if (loadoutId != state->loadout_id) {
        CharacterInitDebugTraceMismatch(packet, "loadout_id");
    }
#else
    UNUSED(packet);
    UNUSED(state);
    UNUSED(profileId);
    UNUSED(loadoutId);
#endif
}

static void CharacterInitTraceTransform(const char* packet, const CharacterInitState* state,
                                        const vec4* position, const vec4* rotation, u32 transientId) {
#if defined(YOTE_INTERNAL)
    if (transientId != state->transient_id) {
        CharacterInitDebugTraceMismatch(packet, "transient_id");
    }
    if (memcmp(position, &state->position, sizeof(vec4)) != 0) {
        CharacterInitDebugTraceMismatch(packet, "position");
    }
    if (memcmp(rotation, &state->rotation, sizeof(vec4)) != 0) {
        CharacterInitDebugTraceMismatch(packet, "rotation");
    }
#else
    UNUSED(packet);
    UNUSED(state);
    UNUSED(position);
    UNUSED(rotation);
    UNUSED(transientId);
#endif
}

// ============================================================================
// SendSelfToClient — Raw binary approach
//
// Loads sendself_patched.bin and patches guid + character_id at runtime.
// The bin was captured from a working server and contains full equipment,
// inventory, profiles, and loadout data that makes the character visible.
//
// Replace your current SendSelfToClient() function in sendSelfToClient.c
// with this one. Keep the existing helper functions (GetActorModelId, etc.)
// ============================================================================

// The original character_id baked into sendself_patched.bin
// Every occurrence of this value gets replaced with the session's actual character_id
#define SENDSELF_BIN_ORIG_CHARID_HI 0x02d8022a
#define SENDSELF_BIN_ORIG_CHARID_LO 0xff9431d1
#define SENDSELF_BIN_ORIG_CHARID    0x02d8022aff9431d1ull

// The guid that was patched in previously (at offset 5)
#define SENDSELF_BIN_ORIG_GUID      0x0000189700002fa7ull

// Temporary compatibility mode: use the captured SendSelf packet layout from
// sendself_patched.bin while we reconcile unknown structured fields.
#define SENDSELF_USE_RAW_PATCHED 0

void SendSelfToClientRaw(AppState* app, SessionState* session) {
    // 1. Load the binary file
    u32 maxBuf = KB(20);
    u8* fileBuffer = arena_push_size(&app->arenaPerTick, maxBuf);

    u32 fileLen = app->api->buffer_load_from_file("..\\data\\sendself_patched.bin", fileBuffer, maxBuf);
    if (!fileLen) {
        printf("[SENDSELF RAW] ERROR: Failed to load sendself_patched.bin!\n");
        return;
    }

    printf("[SENDSELF RAW] Loaded %u bytes from sendself_patched.bin\n", fileLen);
    printf("[SENDSELF RAW] Session guid=0x%llx characterId=0x%llx\n",
           (unsigned long long)session->guid,
           (unsigned long long)session->characterId);

    // 2. Patch the guid at offset 5 (u64 LE)
    endian_write_u64_little(fileBuffer + 5, session->characterId);
    printf("[SENDSELF RAW] Patched guid at offset 5\n");

    // 3. Find and replace ALL occurrences of the original character_id
    //    The original char_id appears 24 times throughout the packet
    u8 origCharIdBytes[8];
    u8 newCharIdBytes[8];
    endian_write_u64_little(origCharIdBytes, SENDSELF_BIN_ORIG_CHARID);
    endian_write_u64_little(newCharIdBytes, session->characterId);

    u32 replacements = 0;
    for (u32 i = 0; i <= fileLen - 8; i++) {
        if (memcmp(fileBuffer + i, origCharIdBytes, 8) == 0) {
            memcpy(fileBuffer + i, newCharIdBytes, 8);
            replacements++;
        }
    }
    printf("[SENDSELF RAW] Replaced character_id %u times\n", replacements);

    // 4. Recalculate stream length (u32 LE at offset 1)
    //    Stream length = total packet length - opcode(1) - stream_length_field(4) = fileLen - 5
    u32 streamLen = fileLen - 5;
    endian_write_u32_little(fileBuffer + 1, streamLen);
    printf("[SENDSELF RAW] Stream length set to %u\n", streamLen);

    // 5. Send it through the gateway tunnel
    //    We need to prepend the tunnel header, same as ZonePacketSend does
    u8* baseBuffer = arena_push_size(&app->arenaPerTick, fileLen + TunnelDataHeaderLen);
    memcpy(baseBuffer + TunnelDataHeaderLen, fileBuffer, fileLen);

    printf("[SENDSELF RAW] Sending %u bytes (+%d header)\n", fileLen, TunnelDataHeaderLen);
    GatewayTunnelDataSend(app, session, baseBuffer, fileLen + TunnelDataHeaderLen);
}

void SendSelfToClient(AppState* app, SessionState* session, int withStats) {
#if SENDSELF_USE_RAW_PATCHED
    (void)withStats;
    SendSelfToClientRaw(app, session);
    return;
#endif

    static int sstcCount = 0;
    sstcCount++;
    printf("[SSTC] SendSelfToClient called %d time(s) total [withStats=%d] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d\n",
           sstcCount, withStats,
           (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased);
    CharacterInitState* initState = 0;
    if (!GetCharacterInitState(session, &initState)) {
        printf("[SENDSELF] aborted: CharacterInitState validation failed\n");
        return;
    }

    u32 actorModelId = initState->actor_model_id;
    u32 gender = initState->gender;
    u32 headType = initState->head_type;
    String8 headActor = initState->head_actor;
    String8 hairModel = initState->hair_model;
    String8 chestModel = CharacterInitFindAttachmentModel(initState, EQUIPMENT_SLOT_CHEST);
    String8 legsModel = CharacterInitFindAttachmentModel(initState, EQUIPMENT_SLOT_LEGS);
    String8 hoodieModel = CharacterInitFindAttachmentModel(initState, 10);
    String8 pantsModel = CharacterInitFindAttachmentModel(initState, LOADOUT_SLOT_LEGS);
    String8 shoesModel = CharacterInitFindAttachmentModel(initState, LOADOUT_SLOT_FEET);

    String8 charName = initState->character_name;

    char identityFallbackBuf[32] = {0};
    u64 steamLikeFallback = 76561197960265728ull + (session->characterId & 0xffffffffull);
    i32 identityFallbackLen = snprintf(identityFallbackBuf, sizeof(identityFallbackBuf), "%llu",
                                       (unsigned long long)steamLikeFallback);
    String8 identityString = session->ticketIdentity;
    if (identityString.size == 0 && identityFallbackLen > 0) {
        identityString.data = (u8*)identityFallbackBuf;
        identityString.size = (u64)identityFallbackLen;
    }

    printf("[SENDSELF] Dynamic packer: guid=0x%llx model=%u gender=%u head=%u name='%.*s' id='%.*s'\n",
           (unsigned long long)session->characterId, actorModelId, gender, headType,
           (int)charName.size, charName.data,
           (int)identityString.size, identityString.data);

    Zone_Packet_SendSelfToClient sendSelf = { 0 };

sendSelf.payload_self = (struct payload_self_s[1]){
        [0] = {
            .guid = initState->character_id,
            .character_id = initState->character_id,
            .transient_id.value = initState->transient_id,
            .last_login_date = 0,
            .actor_model_id = actorModelId,
            .head_actor = headActor,
            .hair_model = hairModel,
            .hair_tint = 0,
            .eye_tint = 0,
            .unk_string_2 = STR8(""),
            .unk_string_31 = STR8(""),
            .unk_string_4 = STR8(""),
            .head_id = headType,
            .unk_u32_3 = 0,
            .faction_id = 2,
            .unk_u32_4 = 0,
            .unk_u32_5 = 0,
            // Position matching ClientBeginZoning
            // .position = {.x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f},
            // .rotation = {.x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f},
            .position = {.x = initState->position.x, .y = initState->position.y, .z = initState->position.z, .w = initState->position.w},
            .rotation = {.x = initState->rotation.x, .y = initState->rotation.y, .z = initState->rotation.z, .w = initState->rotation.w},
            // Identity
            .unk_u32_iden = (u32)(session->characterId & 0xFFFFFFFF),
            .unk_u32_iden_2 = (u32)((session->characterId >> 32) & 0xFFFFFFFF),
            .unk_u32_iden_3 = 0,
            .character_first_name = charName,
            .character_last_name = STR8(""),
            .unk_string_iden = identityString,
            .character_name = charName,
            .unk_qword_1 = 0,
            .unk_u32_6 = 0,
            // Empty arrays
            .currencies_count = 0,
            
            // Missing block added from stream analysis
            .creation_date = 0,
            .unk_u32_7 = 0,
            .unk_u32_8 = 0,
            .unk_bool = FALSE,
            .is_respawning = FALSE, // Moved from post-inventory
            .unk_u32_9 = 0,
            .unk_u32_10 = 0,
            .unk_bool_3 = FALSE,
            .unk_u32_11 = 0,
            .gender1 = gender,      // Moved from post-inventory
            .unk_u32_12 = 0,
            .unk_u32_13 = 0,
            .unk_u32_14 = 0,
            .unk_time1 = 0,
            .unk_time2_2 = 0,
            .unk_u32_15 = 0,
            .unk_bool_5 = FALSE,
            .unk_u32_16 = 0,

            .profiles_count = 1,
            .profiles = (struct profiles_s[1]){
                [0] = {
                    .profile_id = initState->profile_id,
                    .name_id1 = 0,
                    .description_id = 0,
                    .type = 3,
                    .unk_dword_1 = 0,
                    .icon_id = 0,
                    .unk_u32 = 0,
                    .unk_u32_2 = 0,
                    .unk_byte_1 = 0,
                    .unk_byte_2 = 0,
                    .unk_u32_4 = 0,
                    .profile_item_class_data_count = 13,
                    .profile_item_class_data = (struct profile_item_class_data_s[13]){
                        [0]  = { .unk_u32 = 0x0500, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [1]  = { .unk_u32 = 0x0600, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [2]  = { .unk_u32 = 0xE400, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [3]  = { .unk_u32 = 0x0E01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [4]  = { .unk_u32 = 0x0F01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [5]  = { .unk_u32 = 0x1001, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [6]  = { .unk_u32 = 0x1C01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [7]  = { .unk_u32 = 0x3B01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [8]  = { .unk_u32 = 0x4801, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [9]  = { .unk_u32 = 0x8601, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [10] = { .unk_u32 = 0x9501, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [11] = { .unk_u32 = 0xDC01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                        [12] = { .unk_u32 = 0xDF01, .unk_u32_2 = 0, .unk_u32_3 = 0 },
                    },
                    .unk_u32_5 = 0,
                    .unk_u32_6 = 0,
                    .unk_u8 = 0,
                    .unk_f32 = 0.0f,
                    .unk_f32_2 = 0.0f,
                    .unk_f32_3 = 0,
                    .unk_u32_7 = 0,
                    .unk_f32_4 = 0,
                    .unk_u32_8 = 0,
                    .unk_u32_9 = 0,
                    .unk_u32_10 = 0,
                    .unk_u32_11 = 0,
                    .unk_u32_12 = 0,
                },
            },
            .current_profile = initState->profile_id,
            .unk_list_count = 0,
            .collections_count = 0,
            // Inventory
            .items1_count = 5,
            .items1 = (struct items1_s[5]){
                [0] = {
                    .item_def_id1       = 85,     // WEAPON_FISTS
                    .tint_id            = 0,
                    .guid               = ITEM_GUID_FISTS,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = ITEM_GUID_FISTS,
                    .container_def_id   = 1,
                    .container_slot_id  = LOADOUT_SLOT_MELEE,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [1] = {
                    .item_def_id1       = 1542,   // WEAPON_BINOCULARS
                    .tint_id            = 0,
                    .guid               = ITEM_GUID_BINOCULARS,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = ITEM_GUID_BINOCULARS,
                    .container_def_id   = 1,
                    .container_slot_id  = LOADOUT_SLOT_BINOCULARS,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [2] = {
                    .item_def_id1       = ITEM_DEF_HOODIE,   // Hoodie
                    .tint_id            = 0,
                    .guid               = ITEM_GUID_HOODIE,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = ITEM_GUID_HOODIE,
                    .container_def_id   = 1,
                    .container_slot_id  = 10,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [3] = {
                    .item_def_id1       = ITEM_DEF_SKINNY_JEANS,   // Skinny Jeans
                    .tint_id            = 0,
                    .guid               = ITEM_GUID_JEANS,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = ITEM_GUID_JEANS,
                    .container_def_id   = 1,
                    .container_slot_id  = 14,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [4] = {
                    .item_def_id1       = ITEM_DEF_CONVEYS,   // Conveys Sneakers
                    .tint_id            = 0,
                    .guid               = ITEM_GUID_SHOES,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = ITEM_GUID_SHOES,
                    .container_def_id   = 1,
                    .container_slot_id  = 13,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
            },
            .unk_bool_14 = FALSE,
            .ammo_slots1_count = 0,
            .fire_groups1_count = 0,
            .equipment_slot_id1 = EQUIPMENT_SLOT_RIGHT_HAND,
            .unk_byte_2 = 0,
            .unk_dword_73 = 0,
            .unk_byte_31 = 0,
            .unk_byte_4 = 0,
            .unk_byte_5 = 0,
            .unk_float_1 = 0,
            .unk_byte_6 = 0,
            .unk_dword_26 = 0,
            .unk_byte_7 = 0,
            .unk_dword_35 = 0,
            .character_stats1_count = withStats ? 1 : 0,
            .character_stats1 = withStats ? (struct character_stats1_s[1]){
                { .stat_id11 = 2, .stat_id22 = 2, .variable_u8_1_case = 0, .variable_u8_1 = { .vartype_1 = { .base = 1, .modifier = 0 } } },
            } : NULL,
            .unk_array_221_count = 0,
            .unk_dword_10 = 0,
            .gender2 = gender,
            // Quests/achievements/acquaintances/recipes/mounts — all empty
            .quests_count = 0,
            .unk_dword_17 = 0,
            .unk_dword_212 = 0,
            .unk_bool_19 = FALSE,
            .unk_dword_311 = 0,
            .unk_dword_49 = 0,
            .achievements_count = 0,
            .acquaintances_count = 0,
            .recipes_count = 0,
            .mounts_count = 0,
            .send_first_time9_events = 0,
            .unk_array_41_count = 0,
            .unk_array_24_count = 0,
            .unk_effect_array_count = 0,
            .stats3_count = 0,
            .player_titles_count = 0,
            .current_player_title = 0,
            .unk_array_73_count = 0,
            .unk_array_94_count = 0,
            .unk_u32_18 = 0,
            .fire_modes_1_count = 0,
            .fire_modes_2_count = 0,
            .unk_array_117_count = 0,
            .unk_u32_19 = 0,
            .unk_u32_20 = 0,
            .unk_array_13_count = 0,
            .unk_array_25_count = 0,
            .unk_array_32_count = 0,
            .ability_lines_1_count = 0,
            .ability_lines_2_count = 0,
            .ability_lines_3_count = 0,
            .ability_lines_4_count = 0,
            .unk_dword_4238 = 0,
            .unk_dword_240 = 0,
            .unk_array_16_count = 0,
            .unk_array_261_count = 0,
            .unk_array_33_count = 0,
            .unk_array_42_count = 0,
            .unk_array_17_count = 0,
            .unk_array_271_count = 0,
            .unk_byte_9 = 0,
            // unk block 3
            .unk_dword_6110 = 0,
            .unk_dword_6121 = 0,
            .unk_dword_248 = 0,
            .unk_dword_335 = 0,
            .unk_dword_6132 = 0,
            .unk_dword_249 = 0,
            .unk_dword_336 = 0,
            .unk_dword_6143 = 0,
            .unk_dword_3377 = 0,
            .unk_qword_8 = 0,
            .unk_dword_6154 = 0,
            .unk_dword_250 = 0,
            .unk_qword_9 = 0,
            .unk_qword_10 = 0,
            .unk_dword_6165 = 0,
            .unk_dword_251 = 0,
            .unk_dword_338 = 0,
            .unk_dword_6176 = 0,
            .unk_dword_252 = 0,
            .unk_dword_339 = 0,
            .unk_dword_425 = 0,
            .unk_string_32 = STR8(""),
            .unk_byte_10 = 0,
            .unk_array_188_count = 0,
            .unk_dword_6198 = 0,
            .unk_dword_254 = 0,
            .unk_dword_340 = 0,
            .unk_dword_426 = 0,
            .unk_dword_521 = 0,
            .unk_array_199_count = 0,
            .unk_array_280_count = 0,
            .unk_array_291_count = 0,
            .unk_array_2112_count = 0,
            .unk_array_2122_count = 0,
            .equipment_slots_count = 7,
            .equipment_slots = (struct equipment_slots_s[7]){
                [0] = { .unk_dword_7199=1,   .unk_dword_890=1,   .unk_string_4=headActor,               .unk_string_2=STR8("Default"), .equipment_slot_id2=1,   .equipment_slot_id3=1,   .guid=0,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [1] = { .unk_dword_7199=3,   .unk_dword_890=3,   .unk_string_4=chestModel,              .unk_string_2=STR8("Default"), .equipment_slot_id2=3,   .equipment_slot_id3=3,   .guid=0x1001,          .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [2] = { .unk_dword_7199=4,   .unk_dword_890=4,   .unk_string_4=legsModel,               .unk_string_2=STR8("Default"), .equipment_slot_id2=4,   .equipment_slot_id3=4,   .guid=0x1002,          .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [3] = { .unk_dword_7199=7,   .unk_dword_890=7,   .unk_string_4=STR8("Weapon_Empty.adr"), .unk_string_2=STR8("Default"), .equipment_slot_id2=7,   .equipment_slot_id3=7,   .guid=ITEM_GUID_FISTS, .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [4] = { .unk_dword_7199=10,  .unk_dword_890=10,  .unk_string_4=hoodieModel,             .unk_string_2=STR8("Default"), .equipment_slot_id2=10,  .equipment_slot_id3=10,  .guid=ITEM_GUID_HOODIE, .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [5] = { .unk_dword_7199=14,  .unk_dword_890=14,  .unk_string_4=pantsModel,              .unk_string_2=STR8("Default"), .equipment_slot_id2=14,  .equipment_slot_id3=14,  .guid=ITEM_GUID_JEANS,  .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [6] = { .unk_dword_7199=13,  .unk_dword_890=13,  .unk_string_4=shoesModel,              .unk_string_2=STR8("Default"), .equipment_slot_id2=13,  .equipment_slot_id3=13,  .guid=ITEM_GUID_SHOES,  .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
            },
            .unk_array_2135_count = 0,
            .unk_dword_8123 = 0,
            .unk_dword_264 = 0,
            .unk_dword_348 = 0,
            .unk_dword_429 = 0,
            .unk_dword_523 = 0,
            .unk_dword_8134 = 0,
            .unk_dword_265 = 0,
            .unk_dword_349 = 0,
            .implant_slots_count = 0,
            .unk_array_2141_count = 0,
            .unk_array_215_count = 0,
            .unk_array_34_count = 0,
            .unk_array_2166_count = 0,
            .unk_dword_93 = 0,
            .unk_dword_272 = 0,
            .unk_qword_2136 = 0,
            .unk_qword_214 = 0,
            .unk_array_2172_count = 0,
            .unk_dword_96 = 0,
            .unk_dword_275 = 0,
            .unk_qword_2178 = 0,
            .unk_array_218_count = 0,
            .unk_array_35_count = 0,
            .unk_byte_14 = 0,
            .unk_dword_103 = 0,
            .unk_dword_280 = 0,
            .unk_string_5 = STR8(""),
            .unk_array_2193_count = 0,
            .unk_array_2220_count = 0,
            .unk_array_36_count = 0,
            // Loadout — KotK character loadout (profile 17) with default melee + binoculars
            .loadout_id = LOADOUT_ID_KOTK_CHARACTER,
            .loadout_slots_array_count = 2,
            .loadout_slots_array = (struct loadout_slots_array_s[2]){
                [0] = {
                    .hotbar_slot_id    = LOADOUT_SLOT_MELEE,
                    .loadout_id        = LOADOUT_ID_KOTK_CHARACTER,
                    .slot_id           = LOADOUT_SLOT_MELEE,
                    .item_def_id4      = WEAPON_FISTS,
                    .loadout_item_guid = ITEM_GUID_FISTS,
                    .unk_byte_17       = 1,
                    .unk_dword_111     = 22,
                },
                [1] = {
                    .hotbar_slot_id    = LOADOUT_SLOT_BINOCULARS,
                    .loadout_id        = LOADOUT_ID_KOTK_CHARACTER,
                    .slot_id           = LOADOUT_SLOT_BINOCULARS,
                    .item_def_id4      = WEAPON_BINOCULARS,
                    .loadout_item_guid = ITEM_GUID_BINOCULARS,
                    .unk_byte_17       = 1,
                    .unk_dword_111     = 22,
                },
            },
            .current_slot_id = LOADOUT_SLOT_MELEE,
            .unk_array_22537_count = 0,
            .unk_array_22645_count = 0,
            .unk_array_2275_count = 0,
            .unk_array_37_count = 0,
            .unk_array_43_count = 0,
            .unk_array_52_count = 0,
            // Resources
            .character_resources_count = 9,
            .character_resources = (struct character_resources_s[9]){
                [0] = {
                    .resource_type1 = HEALTHTYPE,
                    .resource_id = HEALTHID,
                    .resource_type2 = HEALTHTYPE,
                    .unk_array_22866_count = 0,
                    .value = 10000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [1] = {
                    .resource_type1 = HUNGERTYPE,
                    .resource_id = HUNGERID,
                    .resource_type2 = HUNGERTYPE,
                    .unk_array_22866_count = 0,
                    .value = 10000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [2] = {
                    .resource_type1 = HYDRATIONTYPE,
                    .resource_id = HYDRATIONID,
                    .resource_type2 = HYDRATIONTYPE,
                    .unk_array_22866_count = 0,
                    .value = 10000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [3] = {
                    .resource_type1 = STAMINATYPE,
                    .resource_id = STAMINAID,
                    .resource_type2 = STAMINATYPE,
                    .unk_array_22866_count = 0,
                    .value = 10000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [4] = {
                    .resource_type1 = VIRUSTYPE,
                    .resource_id = VIRUSID,
                    .resource_type2 = VIRUSTYPE,
                    .unk_array_22866_count = 0,
                    .value = 0,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [5] = {
                    .resource_type1 = BLEEDINGTYPE,
                    .resource_id = BLEEDINGID,
                    .resource_type2 = BLEEDINGTYPE,
                    .unk_array_22866_count = 0,
                    .value = 0,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [6] = {
                    .resource_type1 = COMFORTTYPE,
                    .resource_id = COMFORTID,
                    .resource_type2 = COMFORTTYPE,
                    .unk_array_22866_count = 0,
                    .value = 5000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [7] = {
                    .resource_type1 = FUELTYPE,
                    .resource_id = FUELID,
                    .resource_type2 = FUELTYPE,
                    .unk_array_22866_count = 0,
                    .value = 0,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
                [8] = {
                    .resource_type1 = CONDITIONTYPE,
                    .resource_id = CONDITIONID,
                    .resource_type2 = CONDITIONTYPE,
                    .unk_array_22866_count = 0,
                    .value = 10000,
                    .unk_dword_130 = 0, .unk_dword_295 = 0, .unk_dword_360 = 0, .unk_dword_431 = 0, .unk_dword_524 = 0, .unk_dword_622 = 0, .unk_dword_721 = 0, .unk_dword_819 = 0, .unk_qword_41 = 0, .unk_qword_219 = 0, .unk_qword_3 = 0, .unk_qword_4 = 0, .unk_qword_53 = 0, .unk_byte_25 = 0, .unk_byte_2 = 0,
                },
            },
            // Skill points
            .skill_points_granted = 0,
            .skill_points_total = 0,
            .skill_points_spent = 0,
            .unk_qword_42 = 0,
            .unk_qword_220 = 0,
            .unk_dword_131 = 0,
            .skills_count = 0,
            .containers_count = 0,
            .containers = NULL,
            .unk_array_22978_count = 0,
            .unk_array_221089_count = 0,
            .quiz_complete = 0,
            .unk_qword_44 = 0,
            .unk_dword_3628 = 0,
            .vehicle_loadout_related_qword = 0,
            .unk_qword_3 = 0,
            .vehicle_loadout_related_dword = 0,
            .unk_dword_4320 = 0,
            .is_admin = TRUE,
            .first_person_only = 0,
            .spectator_flags = 0,
        }
    };

    struct items1_s itemsFromInit[CHARACTER_INIT_MAX_ITEMS] = { 0 };
    for (u32 i = 0; i < initState->items_count && i < CHARACTER_INIT_MAX_ITEMS; i++) {
        const CharacterInitItem* item = &initState->items[i];
        itemsFromInit[i].item_def_id1 = item->item_def_id;
        itemsFromInit[i].tint_id = 0;
        itemsFromInit[i].guid = item->guid;
        itemsFromInit[i].count = 1;
        itemsFromInit[i].unk_qword_21 = 0;
        itemsFromInit[i].unk_dword_53 = 0;
        itemsFromInit[i].unk_dword_24 = 0;
        itemsFromInit[i].container_guid = item->container_guid;
        itemsFromInit[i].container_def_id = item->container_def_id;
        itemsFromInit[i].container_slot_id = item->container_slot_id;
        itemsFromInit[i].base_durability = 0;
        itemsFromInit[i].current_durability = 0;
        itemsFromInit[i].max_durability_from_def = 0;
        itemsFromInit[i].unk_bool_13 = FALSE;
        itemsFromInit[i].owner_character_id = item->owner_character_id;
        itemsFromInit[i].unk_dword_9 = 0;
    }
    sendSelf.payload_self[0].items1_count = initState->items_count;
    sendSelf.payload_self[0].items1 = itemsFromInit;

    struct equipment_slots_s equipmentFromInit[CHARACTER_INIT_MAX_EQUIPMENT_SLOTS] = { 0 };
    for (u32 i = 0; i < initState->equipment_slots_count && i < CHARACTER_INIT_MAX_EQUIPMENT_SLOTS; i++) {
        const CharacterInitEquipmentSlot* slot = &initState->equipment_slots[i];
        String8 model = CharacterInitFindAttachmentModel(initState, slot->equipment_slot_id);
        equipmentFromInit[i].unk_dword_7199 = slot->equipment_slot_id;
        equipmentFromInit[i].unk_dword_890 = slot->equipment_slot_id;
        equipmentFromInit[i].unk_string_4 = model;
        equipmentFromInit[i].unk_string_2 = STR8("Default");
        equipmentFromInit[i].equipment_slot_id2 = slot->equipment_slot_id;
        equipmentFromInit[i].equipment_slot_id3 = slot->equipment_slot_id;
        equipmentFromInit[i].guid = slot->guid;
        equipmentFromInit[i].tint_alias = STR8("Default");
        equipmentFromInit[i].decal_alias = STR8("#");
    }
    sendSelf.payload_self[0].equipment_slots_count = initState->equipment_slots_count;
    sendSelf.payload_self[0].equipment_slots = equipmentFromInit;

    struct loadout_slots_array_s loadoutFromInit[CHARACTER_INIT_MAX_LOADOUT_SLOTS] = { 0 };
    for (u32 i = 0; i < initState->loadout_slots_count && i < CHARACTER_INIT_MAX_LOADOUT_SLOTS; i++) {
        const CharacterInitLoadoutSlot* slot = &initState->loadout_slots[i];
        loadoutFromInit[i].hotbar_slot_id = slot->hotbar_slot_id;
        loadoutFromInit[i].loadout_id = initState->loadout_id;
        loadoutFromInit[i].slot_id = slot->slot_id;
        loadoutFromInit[i].item_def_id4 = slot->item_def_id;
        loadoutFromInit[i].loadout_item_guid = slot->loadout_item_guid;
        loadoutFromInit[i].unk_byte_17 = 1;
        loadoutFromInit[i].unk_dword_111 = 22;
    }
    sendSelf.payload_self[0].loadout_id = initState->loadout_id;
    sendSelf.payload_self[0].loadout_slots_array_count = initState->loadout_slots_count;
    sendSelf.payload_self[0].loadout_slots_array = loadoutFromInit;
    sendSelf.payload_self[0].current_slot_id = initState->current_loadout_slot_id;

    sendSelf.payload_self[0].profiles[0].profile_id = initState->profile_id;
    sendSelf.payload_self[0].current_profile = initState->profile_id;

    CharacterInitTraceProfileLoadout("SendSelfToClient", initState,
                                     sendSelf.payload_self[0].current_profile,
                                     sendSelf.payload_self[0].loadout_id);
    CharacterInitTraceTransform("SendSelfToClient", initState,
                                &sendSelf.payload_self[0].position,
                                &sendSelf.payload_self[0].rotation,
                                sendSelf.payload_self[0].transient_id.value);

    printf("[SENDSELF] item_guid summary: fists=0x%llx bino=0x%llx hoodie=0x%llx jeans=0x%llx shoes=0x%llx eyes=0x%llx\n",
            (unsigned long long)ITEM_GUID_FISTS,
            (unsigned long long)ITEM_GUID_BINOCULARS,
            (unsigned long long)ITEM_GUID_HOODIE,
            (unsigned long long)ITEM_GUID_JEANS,
            (unsigned long long)ITEM_GUID_SHOES,
            (unsigned long long)ITEM_GUID_EYES_ATTACHMENT);

    // Use the debug version that hex-dumps the packed data
    // This will show us the stream:u32 length prefix and verify it matches
    ZonePacketSendSelfDebug(app, session, &app->arenaPerTick,
                            Zone_Packet_Kind_SendSelfToClient, &sendSelf);
}
