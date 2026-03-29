// ============================================================================
// Hex dump helper
// ============================================================================
void HexDumpBuffer(const char* label, u8* data, u32 len) {
    printf("\n[HEX DUMP] %s (%u bytes):\n", label, len);
    for (u32 i = 0; i < len && i < 64; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len > 64) printf("... (%u more bytes)", len - 64);
    printf("\n\n");
}

void ZonePacketSendDebug(AppState* app, SessionState* session, Arena* arena, Zone_Packet_Kind kind,
                         void* packetPtr, const char* label) {
    u8* baseBuffer = arena_push_size(arena, MAX_PACKET_LENGTH);
    u8* packedBuffer = baseBuffer + TunnelDataHeaderLen;
    u32 packedLen = zone_packet_pack(kind, packetPtr, packedBuffer);
    printf("[ZONE SEND DEBUG] %s kind=%d packedLen=%u\n", label, kind, packedLen);
    HexDumpBuffer(label, packedBuffer, packedLen);
    u32 totalLen = packedLen + TunnelDataHeaderLen;
    GatewayTunnelDataSend(app, session, baseBuffer, totalLen);
}


// ============================================================================
// Phase 2: Deploy character after client sends ClientIsReady (0x04).
//
// KEY FIX: SendSelfToClient is re-sent here. The Phase 1 copy was consumed
// by the pre-zone context (main menu). After ClientBeginZoning the client
// enters a new zone context and needs ALL initial data re-sent in that
// context. Without this, InitialZoneDataComplete never flips to 1.
// ============================================================================
void DeployCharacter(AppState* app, SessionState* session) {
    __time64_t timer;
    _time64(&timer);

    // Reset loading flags for this new phase so the client can trigger them once more
    session->finished_loading = FALSE;
    session->isReady = FALSE;

    printf("\n========== DEPLOY CHARACTER BEGIN ==========\n");

    // 1. RE-SEND SendSelfToClient in post-zone context
    printf("[DEPLOY] Step 1: Re-sending SendSelfToClient in post-zone context\n");
    SendSelfToClient(app, session, TRUE);

    // 2. Update initial location
    Zone_Packet_ClientUpdate_UpdateLocation updateLoc = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = FALSE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLoc);

    // 3. Container Initialization
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id = session->characterId;
    containers.container_list_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 4. Reference Data
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096, "data/ReferenceData_ProfileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 8192, "data/ReferenceData_ProjectileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096, "data/ReferenceData_ItemClassDefinitions.bin");

    // 5. AddLightweightPc — Spawns the model (MUST be before done signals)
    Zone_Packet_AddLightweightPc addPc = { 0 };
    addPc.character_id = session->characterId;
    addPc.transient_id.value = 52;
    addPc.id_characterFirstName = session->characterName;
    addPc.id_characterLastName = STR8("");
    addPc.id_unknownString1 = STR8("");
    addPc.id_characterName = session->characterName;
    addPc.actorModelId = session->pGetPlayerActor.actorModelId ? session->pGetPlayerActor.actorModelId : 9469;
    addPc.position.x = -297.31f;
    addPc.position.y = 506.06f;
    addPc.position.z = -4894.10f;
    addPc.rotation.x = 0.0f;
    addPc.rotation.y = -0.7071f;
    addPc.rotation.z = 0.0f;
    addPc.rotation.w = 0.7071f;
    addPc.movementVersion = 1;
    addPc.flags1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_AddLightweightPc, &addPc);

    // 6. Equipment — Chest + Legs + Fists
    Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    setEquipment.unk_string_1 = STR8("Default");
    setEquipment.unk_string_2 = STR8("#");
    setEquipment.unk_bool_2 = TRUE;
    setEquipment.length_1 = (struct length_1_s[1]){[0] = {
        .character_id = session->characterId, 
        .profile_id = 5,
    }};
    setEquipment.equipment_slot_array_count = 3;
    setEquipment.equipment_slot_array = (struct equipment_slot_array_s[3]){
        [0] = { .equipment_slot_id_1 = 3, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3, .guid = 0x1001, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 4, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4, .guid = 0x1002, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [2] = { .equipment_slot_id_1 = 7, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7, .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    };
    setEquipment.attachments_data_1_count = 3;
    setEquipment.attachments_data_1 = (struct attachments_data_1_s[3]){
        [0] = { .model_name = STR8("SurvivorMale_Chest_Bra.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3 },
        [1] = { .model_name = STR8("SurvivorMale_Legs_Pants_Underwear.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4 },
        [2] = { .model_name = STR8("Weapon_Empty.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7 },
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);

    // 7. Loadout — Fists + Binoculars
    Zone_Packet_Loadout_SetLoadoutSlots setLoadoutSlots = { 0 };
    setLoadoutSlots.character_id = session->characterId;
    setLoadoutSlots.loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    setLoadoutSlots.loadout_slot_data_count = 2;
    setLoadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[2]){
        [0] = { .hotbar_slot_id = LOADOUT_SLOT_MELEE, .loadout_id_1 = LOADOUT_ID_KOTK_CHARACTER, .slot_id = LOADOUT_SLOT_MELEE, .item_def_id1 = WEAPON_FISTS, .loadout_item_guid = ITEM_GUID_FISTS },
        [1] = { .hotbar_slot_id = LOADOUT_SLOT_BINOCULARS, .loadout_id_1 = LOADOUT_ID_KOTK_CHARACTER, .slot_id = LOADOUT_SLOT_BINOCULARS, .item_def_id1 = WEAPON_BINOCULARS, .loadout_item_guid = ITEM_GUID_BINOCULARS },
    };
    setLoadoutSlots.current_slot_id = LOADOUT_SLOT_MELEE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Loadout_SetLoadoutSlots, &setLoadoutSlots);

    // 8. Weapon Stance & State Delta
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 8b. UpdateCharacterState — marks character as alive/renderable (all states = 0 = alive)
    Zone_Packet_Character_UpdateCharacterState stateUpdate = { 0 };
    stateUpdate.character_id = session->characterId;
    stateUpdate.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &stateUpdate);

    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    // 9. GameTimeSync
    Zone_Packet_GameTimeSync gameTimeSync = { 0 };
    gameTimeSync.cycle_speed = 12.f;
    gameTimeSync.time = timer;
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_GameTimeSync, &gameTimeSync);

      // 10. DoneSendingPreloadCharacters — immediate
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // 11. ZoneDoneSendingInitialData — immediate (client needs this to exit loading)
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    // 12. Defer ONLY NetworkProximityUpdatesComplete — 5 seconds
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;
    session->characterReleased = TRUE;

    printf("[DEFER] tickCount=%llu, proximityCompleteTick=%llu\n",
           (unsigned long long)*app->tickCount, (unsigned long long)session->proximityCompleteTick);
    printf("========== DEPLOY CHARACTER END ==========\n\n");
}


void OnLogin(AppState* app, SessionState* session) {
    Zone_Packet_InitializationParameters init_params = {
    .environment  = STR8("LIVE_KOTK"),
    .unk_string_1 = STR8("SKU_Is_KotK"),  // was absent
    .ruleset_definitions_count = 0,
};
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    Zone_Packet_SendZoneDetails send_zone_details = {
        .zone_name = STR8("Z2"),
        .zone_type = 4,
        .unk_bool = FALSE,
        .overcast = 1.0f,
        .fogDensity = 0.000173f,
        .fogFloor = 10.0f,
        .fogGradient = 0.0144f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity = 0,
        .cloudWeight0 = 0.05f,
        .cloudWeight1 = 0.0f,
        .cloudWeight2 = 0.05f,
        .cloudWeight3 = 0.15f,
        .transitionTime = 0,
        .sunAxisX = 38,
        .sunAxisY = -15,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = -1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8("sky_Z_clouds.dds"),
        .stratusCloudTiling = 0.30f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight = 1000,
        .cumulusCloudTiling = 0.20f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness = 0.25f,
        .cloudSilverLiningBrightness = 7.0f,
        .cloudShadows = 0.5f,
        .zone_id = 5,
        .zone_id_2 = 5,
        .name_id = 61609,
        .unk_bool2 = TRUE,
        .lighting = STR8("Lighting_Z2.txt"),
        .unk_bool3 = FALSE,
        .unk_bool4 = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendZoneDetails,
                   &send_zone_details);

    Zone_Packet_ClientGameSettings game_settings = {
        .interact_glow_and_dist = 16,
        .unk_bool = TRUE,
        .timescale = 1.0,
        .enable_weapons = 1,
        .unk_u32_2 = 1,
        .unk_float2 = 15.,
        .damage_multiplier = 11.,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientGameSettings,
                   &game_settings);

    // Reference data — must be sent before SendSelfToClient
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 8192,  "data/Command_ItemDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 65536, "data/ReferenceData_WeaponDefinitions.bin");


    Zone_Packet_UpdateWeatherData updt_weather_data = {
        .overcast = 1.0f,
        .fogDensity = 0.000173f,
        .fogFloor = 10.0f,
        .fogGradient = 0.0144f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity = 0,
        .cloudWeight0 = 0.05f,
        .cloudWeight1 = 0.0f,
        .cloudWeight2 = 0.05f,
        .cloudWeight3 = 0.15f,
        .transitionTime = 0,
        .sunAxisX = 38,
        .sunAxisY = -15,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = -1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8("sky_Z_clouds.dds"),
        .stratusCloudTiling = 0.30f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight = 1000,
        .cumulusCloudTiling = 0.20f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness = 0.25f,
        .cloudSilverLiningBrightness = 7.0f,
        .cloudShadows = 0.5f,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_UpdateWeatherData,
                   &updt_weather_data);

    printf("[DEBUG] characterName: '%.*s' len=%d\n",
       (int)session->characterName.size,
       session->characterName.data,
       (int)session->characterName.size);
    printf("[DEBUG] characterId: 0x%llx\n", (unsigned long long)session->characterId);

    // Phase 1: SendSelfToClient before ClientBeginZoning
    // This will be re-sent in DeployCharacter after zone load completes.
    SendSelfToClient(app, session, FALSE);

    // Reset loading flags before zone transition so both phases get one shot each
    session->finished_loading = FALSE;
    session->isReady = FALSE;

    // ClientBeginZoning — triggers zone load
    Zone_Packet_ClientBeginZoning beginZoning = { 0 };
    beginZoning.zone_name                    = STR8("Z2");
    beginZoning.zone_type                    = 4;
    beginZoning.pos                          = (vec4){ .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f };
    beginZoning.rot                          = (vec4){ .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f };
    beginZoning.overcast                     = 1.0f;
    beginZoning.fogDensity                   = 0.000173f;
    beginZoning.fogFloor                     = 10.0f;
    beginZoning.fogGradient                  = 0.0144f;
    beginZoning.globalPrecipitation          = 0.0f;
    beginZoning.temperature                  = 75.0f;
    beginZoning.skyClarity                   = 0.0f;
    beginZoning.cloudWeight0                 = 0.05f;
    beginZoning.cloudWeight1                 = 0.0f;
    beginZoning.cloudWeight2                 = 0.05f;
    beginZoning.cloudWeight3                 = 0.15f;
    beginZoning.transitionTime               = 0.0f;
    beginZoning.sunAxisX                     = 38.0f;
    beginZoning.sunAxisY                     = -15.0f;
    beginZoning.sunAxisZ                     = 0.0f;
    beginZoning.windDirX                     = -1.0f;
    beginZoning.windDirY                     = -0.5f;
    beginZoning.windDirZ                     = -1.0f;
    beginZoning.wind                         = 3.0f;
    beginZoning.rainMinStrength              = 0.0f;
    beginZoning.rainRampUpTimeSeconds        = 1.0f;
    beginZoning.cloudFile                    = STR8("sky_Z_clouds.dds");
    beginZoning.stratusCloudTiling           = 0.30f;
    beginZoning.stratusCloudScrollU          = -0.002f;
    beginZoning.stratusCloudScrollV          = 0.0f;
    beginZoning.stratusCloudHeight           = 1000.0f;
    beginZoning.cumulusCloudTiling           = 0.20f;
    beginZoning.cumulusCloudScrollU          = 0.0f;
    beginZoning.cumulusCloudScrollV          = 0.002f;
    beginZoning.cumulusCloudHeight           = 8000.0f;
    beginZoning.cloudAnimationSpeed          = 0.0f;
    beginZoning.cloudSilverLiningThickness   = 0.25f;
    beginZoning.cloudSilverLiningBrightness  = 7.0f;
    beginZoning.cloudShadows                 = 0.5f;
    beginZoning.unk_byte_1                   = 4;
    beginZoning.zone_id_1                    = 5;
    beginZoning.zone_id_2                    = 5;
    beginZoning.name_id                      = 61609;
    beginZoning.unk_dword_1                  = 0x0f2b07d0;
    beginZoning.unk_bool_1                   = FALSE;
    beginZoning.wait_for_zone_ready          = FALSE;
    beginZoning.unk_bool_2                   = FALSE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

     Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -1220.0f, .y = 50.0f, .z = -1220.0f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = TRUE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientInitializationDetails, &initDetails);
}