u32 GetActorModelId(SessionState* session) {
    u32 headId = session->pGetPlayerActor.headType;

    switch (headId) {
        case 1: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 2: {
            session->pGetPlayerActor.actorModelId = 9240;
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
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 6: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 7: {
            session->pGetPlayerActor.actorModelId = 9240;
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
        case 9240: {
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
        case 9240:
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

void SendSelfToClient(AppState* app, SessionState* session) {
    // Use session data if available, fallback to defaults
    u32 actorModelId = session->pGetPlayerActor.actorModelId;
    if (actorModelId == 0) actorModelId = 9240; // default male

    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1; // default male

    u32 headType = session->pGetPlayerActor.headType;
    if (headType == 0) headType = 1;

    String8 headActor = session->pGetPlayerActor.headActor;
    if (headActor.size == 0) headActor = STR8("SurvivorMale_Head_01.adr");

    String8 hairModel = session->pGetPlayerActor.hairModel;
    if (hairModel.size == 0) hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");

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
                .unk_f32 = 0,
                .unk_f32_2 = 0,
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
        .character_stats1_count = 0,
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
        .equipment_slots_count = 0,
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
        // Loadout
        .loadout_id = 3,
        .loadout_slots_array_count = 0,
        .current_slot_id = 7,
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

    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendSelfToClient,
                   &sendSelf);
}