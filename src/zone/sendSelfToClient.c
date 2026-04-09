// ============================================================================
// SendSelfToClient — Character initialization packet builder
//
// Based on H1emu/h1z1-server ZoneServer2016 sendCharacterData() flow.
// This is the most critical zone packet: it defines the entire player state
// sent to the client during login.
//
// The packet contains (in order):
//   1. Character identity (guid, character_id, name, position, rotation)
//   2. Character appearance (actor_model_id, head_actor, hair_model, gender)
//   3. Profiles (movement speed, type)
//   4. Inventory items (items1 array — MUST match equipment/loadout GUIDs)
//   5. Equipment slots (equipment_slots array)
//   6. Loadout slots (loadout_slots_array — hotbar assignments)
//   7. Resources (health, hunger, hydration, stamina, etc.)
//   8. Skill points, containers, misc flags
//
// Key invariant: Every GUID referenced in equipment_slots and
// loadout_slots_array MUST have a corresponding entry in items1.
// Without this, the client cannot resolve items and the hotbar stays blank.
// ============================================================================

// Resource type helper — maps ResourceId → ResourceType
u32 getResourceType(u32 resourceId) {
    switch (resourceId) {
        case HEALTHID:    return HEALTHTYPE;
        case HUNGERID:    return HUNGERTYPE;
        case HYDRATIONID: return HYDRATIONTYPE;
        case STAMINAID:   return STAMINATYPE;
        case VIRUSID:     return VIRUSTYPE;
        case BLEEDINGID:  return BLEEDINGTYPE;
        case COMFORTID:   return COMFORTTYPE;
        case FUELID:      return FUELTYPE;
        case CONDITIONID: return CONDITIONTYPE;
        default:          return 0;
    }
}

// ============================================================================
// SendSelfToClientRaw — DEPRECATED binary approach
//
// This was a workaround that loaded sendself_patched.bin and patched GUIDs
// at runtime. It is kept for reference but should NOT be used.
// The structured SendSelfToClient() below is the correct implementation.
// ============================================================================
#if 0
void SendSelfToClientRaw(AppState* app, SessionState* session) {
    // See git history for the original implementation
}
#endif

// ============================================================================
// SendSelfToClient — Builds and sends the full character initialization packet
//
// Based on H1emu ZoneServer2016.sendCharacterData()
//
// Parameters:
//   withStats - If TRUE, includes character_stats1 (stat_id=2/movement)
//
// The function:
//   1. Resolves appearance from session data (with fallbacks)
//   2. Builds identity block (guid, character_id, name, position)
//   3. Builds profile block (profile_id=4 for KotK player)
//   4. Builds inventory items (items1 array: fists, binoculars, clothing)
//   5. Builds equipment slots (head, chest, legs, weapon, eyes, etc.)
//   6. Builds loadout slots (hotbar: melee=fists, binoculars, clothing)
//   7. Builds resources (health, hunger, hydration, stamina, etc.)
//   8. Sends via ZonePacketSendSelfDebug for stream validation
// ============================================================================
void SendSelfToClient(AppState* app, SessionState* session, int withStats) {
    printf("[SENDSELF] Building SendSelfToClient (withStats=%d)\n", withStats);

    // Resolve appearance with fallback defaults
    u32 actorModelId = session->pGetPlayerActor.actorModelId;
    if (actorModelId == 0) actorModelId = 9469;

    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1;

    u32 headType = session->pGetPlayerActor.headType;
    if (headType == 0) headType = 1;

    String8 headActor = session->pGetPlayerActor.headActor;
    if (headActor.size == 0) headActor = STR8("SurvivorMale_Head_03.adr");

    String8 hairModel = session->pGetPlayerActor.hairModel;
    if (hairModel.size == 0) hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");

    // Gender-specific appearance models
    String8 eyesModel  = (gender == 2) ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");
    String8 chestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 legsModel  = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");

    String8 charName = session->characterName;
    if (charName.size == 0) charName = STR8("Unknown");

    // Build identity string (Steam64-like fallback if ticket identity is missing)
    char identityFallbackBuf[32] = {0};
    u64 steamLikeFallback = 76561197960265728ull + (session->characterId & 0xffffffffull);
    i32 identityFallbackLen = snprintf(identityFallbackBuf, sizeof(identityFallbackBuf), "%llu",
                                       (unsigned long long)steamLikeFallback);
    String8 identityString = session->ticketIdentity;
    if (identityString.size == 0 && identityFallbackLen > 0) {
        identityString.data = (u8*)identityFallbackBuf;
        identityString.size = (u64)identityFallbackLen;
    }

    printf("[SENDSELF] guid=0x%llx model=%u gender=%u head=%u name='%.*s'\n",
           (unsigned long long)session->characterId, actorModelId, gender, headType,
           (int)charName.size, charName.data);

    // ========================================================================
    // Build the SendSelfToClient packet
    // ========================================================================
    Zone_Packet_SendSelfToClient sendSelf = { 0 };

sendSelf.payload_self = (struct payload_self_s[1]){
        [0] = {
            .guid = session->characterId,
            .character_id = session->characterId,
            .transient_id.value = 1,
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
            .position = {.x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f},
            .rotation = {.x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f},
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
                    .profile_id = 4,
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
                    .unk_f32 = 1.7f,    // Moved to correct offset
                    .unk_f32_2 = 0.95f, // Moved to correct offset
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
            .current_profile = 4,
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
                    .container_def_id   = 85,
                    .container_slot_id  = 7,      // LOADOUT_SLOT_MELEE
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
                    .container_def_id   = 1542,
                    .container_slot_id  = 5,      // LOADOUT_SLOT_BINOCULARS
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [2] = {
                    .item_def_id1       = 5747,   // Hoodie
                    .tint_id            = 0,
                    .guid               = 0x1005,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = 0x1005,
                    .container_def_id   = 5747,
                    .container_slot_id  = 10,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [3] = {
                    .item_def_id1       = 2178,   // Jeans
                    .tint_id            = 0,
                    .guid               = 0x1006,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = 0x1006,
                    .container_def_id   = 2178,
                    .container_slot_id  = 14,
                    .base_durability    = 0,
                    .current_durability = 0,
                    .max_durability_from_def = 0,
                    .unk_bool_13        = FALSE,
                    .owner_character_id = session->characterId,
                    .unk_dword_9        = 0,
                },
                [4] = {
                    .item_def_id1       = 2216,   // Sneakers
                    .tint_id            = 0,
                    .guid               = 0x1007,
                    .count              = 1,
                    .unk_qword_21       = 0,
                    .unk_dword_53       = 0,
                    .unk_dword_24       = 0,
                    .container_guid     = 0x1007,
                    .container_def_id   = 2216,
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
            .equipment_slot_id1 = LOADOUT_SLOT_MELEE,
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
            .equipment_slots_count = 9,
            .equipment_slots = (struct equipment_slots_s[9]){
                [0] = { .unk_dword_7199=15,  .unk_dword_890=15,  .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=15,  .equipment_slot_id3=15,  .guid=0,                    .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [1] = { .unk_dword_7199=27,  .unk_dword_890=27,  .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=27,  .equipment_slot_id3=27,  .guid=0,                    .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [2] = { .unk_dword_7199=3,   .unk_dword_890=3,   .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=3,   .equipment_slot_id3=3,   .guid=0x1005,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [3] = { .unk_dword_7199=4,   .unk_dword_890=4,   .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=4,   .equipment_slot_id3=4,   .guid=0x1006,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [4] = { .unk_dword_7199=7,   .unk_dword_890=7,   .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=7,   .equipment_slot_id3=7,   .guid=ITEM_GUID_FISTS,      .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [5] = { .unk_dword_7199=105, .unk_dword_890=105, .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=105, .equipment_slot_id3=105, .guid=0,                    .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [6] = { .unk_dword_7199=10,  .unk_dword_890=10,  .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=10,  .equipment_slot_id3=10,  .guid=0x1005,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [7] = { .unk_dword_7199=14,  .unk_dword_890=14,  .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=14,  .equipment_slot_id3=14,  .guid=0x1006,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
                [8] = { .unk_dword_7199=13,  .unk_dword_890=13,  .unk_string_4=STR8("Default"), .unk_string_2=STR8("#"), .equipment_slot_id2=13,  .equipment_slot_id3=13,  .guid=0x1007,               .tint_alias=STR8("Default"), .decal_alias=STR8("#") },
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
            .loadout_slots_array_count = 5,
            .loadout_slots_array = (struct loadout_slots_array_s[5]){
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
                [2] = {
                    .hotbar_slot_id    = 10,
                    .loadout_id        = LOADOUT_ID_KOTK_CHARACTER,
                    .slot_id           = 10,
                    .item_def_id4      = 5747,
                    .loadout_item_guid = 0x1005,
                    .unk_byte_17       = 1,
                    .unk_dword_111     = 22,
                },
                [3] = {
                    .hotbar_slot_id    = 14,
                    .loadout_id        = LOADOUT_ID_KOTK_CHARACTER,
                    .slot_id           = 14,
                    .item_def_id4      = 2178,
                    .loadout_item_guid = 0x1006,
                    .unk_byte_17       = 1,
                    .unk_dword_111     = 22,
                },
                [4] = {
                    .hotbar_slot_id    = 13,
                    .loadout_id        = LOADOUT_ID_KOTK_CHARACTER,
                    .slot_id           = 13,
                    .item_def_id4      = 2216,
                    .loadout_item_guid = 0x1007,
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

    // Send via debug wrapper that validates stream length
    ZonePacketSendSelfDebug(app, session, &app->arenaPerTick,
                            Zone_Packet_Kind_SendSelfToClient, &sendSelf);
    printf("[SENDSELF] SendSelfToClient complete\n");
}