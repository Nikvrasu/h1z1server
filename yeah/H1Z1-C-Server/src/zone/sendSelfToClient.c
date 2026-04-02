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
    static int sstcCount = 0;
    sstcCount++;
    printf("[SSTC] SendSelfToClient called %d time(s) total [withStats=%d] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d\n",
           sstcCount, withStats,
           (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased);
    // Use session data if available, fallback to defaults
    u32 actorModelId = session->pGetPlayerActor.actorModelId;
    if (actorModelId == 0) actorModelId = 9469; // default male

    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1; // default male

    u32 headType = session->pGetPlayerActor.headType;
    if (headType == 0) headType = 1;

    String8 headActor = session->pGetPlayerActor.headActor;
    if (headActor.size == 0) headActor = STR8("SurvivorMale_Head_01.adr");

    String8 hairModel = session->pGetPlayerActor.hairModel;
    if (hairModel.size == 0) hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");

    String8 eyesModel = (gender == 2) ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");
    String8 chestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 legsModel = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");

    String8 charName = session->characterName;
    if (charName.size == 0) charName = STR8("Unknown");

    printf("[SENDSELF] Dynamic packer: guid=0x%llx model=%u gender=%u head=%u name='%.*s'\n",
           (unsigned long long)session->characterId, actorModelId, gender, headType,
           (int)charName.size, charName.data);

    Zone_Packet_SendSelfToClient sendSelf = { 0 };

    sendSelf.payload_self = (struct payload_self_s[1]){
    [0] = {
        .guid = session->characterId,
        .character_id = session->characterId,
        .transient_id.value = 52,
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
        .faction_id = 0,
        .unk_u32_4 = 0,
        .unk_u32_5 = 0,
        // Position matching ClientBeginZoning
        .position = {.x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f},
        .rotation = {.x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f},
        // Identity
        .unk_u32_iden = 0,
        .unk_u32_iden_2 = 0,
        .unk_u32_iden_3 = 0,
        .character_first_name = charName,
        .character_last_name = STR8(""),
        .unk_string_iden = STR8("00000000000000000"),
        .character_name = charName,
        .unk_qword_1 = 0,
        .unk_u32_6 = 0,
        // Empty arrays
        .currencies_count = 0,
        .profiles_count = 1,
        .profiles = (struct profiles_s[1]){
            [0] = {
                .profile_id = 5,
                .name_id1 = 0,
                .description_id = 0,
                .type = 3,
                .unk_f32 = 1.7f,
                .unk_f32_2 = 0.95f,
                .unk_dword_1 = 0,
                .icon_id = 0,
                .unk_u32 = 0,
                .unk_u32_2 = 0,
                .unk_byte_1 = 0,
                .unk_byte_2 = 0,
                .unk_u32_4 = 0,
                .profile_item_class_data_count = 0,
                .unk_u32_5 = 0,
                .unk_u32_6 = 0,
                .unk_u8 = 0,
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
        .current_profile = 5,
        .unk_list_count = 0,
        .collections_count = 0,
        // Inventory
        .items1_count = 0,
        .unk_bool_14 = FALSE,
        .ammo_slots1_count = 0,
        .fire_groups1_count = 0,
        .equipment_slot_id1 = 0,
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
        .character_stats1_count = withStats ? 0 : 0,
        .character_stats1 = withStats ? (struct character_stats1_s[1]){
            { .stat_id11 = 2, .stat_id22 = 2, .variable_u8_1_case = 0, .variable_u8_1 = { .vartype_1 = { .base = 1, .modifier = 0 } } },
        } : NULL,
        .unk_array_221_count = 0,
        .unk_dword_10 = 0,
        .is_respawning = FALSE,
        .gender1 = gender,
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
        .equipment_slots_count = 5,
        .equipment_slots = (struct equipment_slots_s[5]){
            [0] = {
                .unk_dword_7199 = 1,
                .unk_dword_890 = 1,
                .unk_string_4 = headActor,
                .unk_string_2 = STR8("Default"),
                .equipment_slot_id2 = 1,
                .equipment_slot_id3 = 1,
                .guid = 0x1003,
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
            [1] = {
                .unk_dword_7199 = 3,
                .unk_dword_890 = 3,
                .unk_string_4 = chestModel,
                .unk_string_2 = STR8("Default"),
                .equipment_slot_id2 = 3,
                .equipment_slot_id3 = 3,
                .guid = 0x1001,
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
            [2] = {
                .unk_dword_7199 = 4,
                .unk_dword_890 = 4,
                .unk_string_4 = legsModel,
                .unk_string_2 = STR8("Default"),
                .equipment_slot_id2 = 4,
                .equipment_slot_id3 = 4,
                .guid = 0x1002,
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
            [3] = {
                .unk_dword_7199 = 7,
                .unk_dword_890 = 7,
                .unk_string_4 = STR8("Weapon_Empty.adr"),
                .unk_string_2 = STR8("Default"),
                .equipment_slot_id2 = 7,
                .equipment_slot_id3 = 7,
                .guid = ITEM_GUID_FISTS,
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
            [4] = {
                .unk_dword_7199 = 105,
                .unk_dword_890 = 105,
                .unk_string_4 = eyesModel,
                .unk_string_2 = STR8("Default"),
                .equipment_slot_id2 = 105,
                .equipment_slot_id3 = 105,
                .guid = 0x1004,
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
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
                // Fists at melee slot (slot 7)
                .hotbar_slot_id   = LOADOUT_SLOT_MELEE,
                .loadout_id       = LOADOUT_ID_KOTK_CHARACTER,
                .slot_id          = LOADOUT_SLOT_MELEE,
                .item_def_id4     = WEAPON_FISTS,
                .loadout_item_guid = ITEM_GUID_FISTS,
                .unk_byte_17      = 0,
                .unk_dword_111    = 0,
            },
            [1] = {
                // Binoculars at binoculars slot (slot 5)
                .hotbar_slot_id   = LOADOUT_SLOT_BINOCULARS,
                .loadout_id       = LOADOUT_ID_KOTK_CHARACTER,
                .slot_id          = LOADOUT_SLOT_BINOCULARS,
                .item_def_id4     = WEAPON_BINOCULARS,
                .loadout_item_guid = ITEM_GUID_BINOCULARS,
                .unk_byte_17      = 0,
                .unk_dword_111    = 0,
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
                .value = 10000,
                .unk_array_22866_count = 0,
            },
            [1] = {
                .resource_type1 = HUNGERTYPE,
                .resource_id = HUNGERID,
                .resource_type2 = HUNGERTYPE,
                .value = 10000,
                .unk_array_22866_count = 0,
            },
            [2] = {
                .resource_type1 = HYDRATIONTYPE,
                .resource_id = HYDRATIONID,
                .resource_type2 = HYDRATIONTYPE,
                .value = 10000,
                .unk_array_22866_count = 0,
            },
            [3] = {
                .resource_type1 = STAMINATYPE,
                .resource_id = STAMINAID,
                .resource_type2 = STAMINATYPE,
                .value = 10000,
                .unk_array_22866_count = 0,
            },
            [4] = {
                .resource_type1 = VIRUSTYPE,
                .resource_id = VIRUSID,
                .resource_type2 = VIRUSTYPE,
                .value = 0,
                .unk_array_22866_count = 0,
            },
            [5] = {
                .resource_type1 = BLEEDINGTYPE,
                .resource_id = BLEEDINGID,
                .resource_type2 = BLEEDINGTYPE,
                .value = 0,
                .unk_array_22866_count = 0,
            },
            [6] = {
                .resource_type1 = COMFORTTYPE,
                .resource_id = COMFORTID,
                .resource_type2 = COMFORTTYPE,
                .value = 5000,
                .unk_array_22866_count = 0,
            },
            [7] = {
                .resource_type1 = FUELTYPE,
                .resource_id = FUELID,
                .resource_type2 = FUELTYPE,
                .value = 0,
                .unk_array_22866_count = 0,
            },
            [8] = {
                .resource_type1 = CONDITIONTYPE,
                .resource_id = CONDITIONID,
                .resource_type2 = CONDITIONTYPE,
                .value = 10000,
                .unk_array_22866_count = 0,
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

    // Use the debug version that hex-dumps the packed data
    // This will show us the stream:u32 length prefix and verify it matches
    ZonePacketSendSelfDebug(app, session, &app->arenaPerTick,
                            Zone_Packet_Kind_SendSelfToClient, &sendSelf);
}