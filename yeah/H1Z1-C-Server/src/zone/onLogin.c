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
// SendEquipmentAndMovement — called from ClientFinishedLoading
// This matches h1emu which sends Equipment, WeaponStance, RunSpeed,
// and ModifyMovementSpeed AFTER the client reports finished loading.
// ============================================================================
void SendEquipmentAndMovement(AppState* app, SessionState* session) {
    printf("\n========== SEND EQUIPMENT & MOVEMENT (ClientFinishedLoading) ==========\n");

    // 1. UpdateWeatherData (h1emu sends this in ClientFinishedLoading)
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

    // 2. Character.WeaponStance
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 3. Equipment.SetCharacterEquipment — Chest + Legs + Fists
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
        [0] = { .equipment_slot_id_1 = 3, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3, .guid = ITEM_GUID_CHEST_CLOTHING, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 4, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4, .guid = ITEM_GUID_LEGS_CLOTHING, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
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

    // 4. Command.RunSpeed
    Zone_Packet_Command_RunSpeed runSpeed = { .run_speed = 0.0f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Command_RunSpeed, &runSpeed);

    // 5. ClientUpdate.ModifyMovementSpeed
    Zone_Packet_ClientUpdate_ModifyMovementSpeed moveSpeed = { 0 };
    moveSpeed.speed = 2.0f;
    moveSpeed.movementVersion = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ModifyMovementSpeed, &moveSpeed);

    printf("========== SEND EQUIPMENT & MOVEMENT END ==========\n\n");
}


// ============================================================================
// Phase 2: Deploy character after client sends ClientIsReady (0x04).
//
// RESTRUCTURED to match h1emu's exact proven sequence:
//   - NO re-send of SendSelfToClient (h1emu sends it only once in OnLogin)
//   - NO AddLightweightPc for self (h1emu never sends this for the local player)
//   - Character.UpdateCharacterState is sent here (h1emu does this)
//   - Equipment/WeaponStance/RunSpeed are NOT sent here (moved to ClientFinishedLoading)
// ============================================================================
void DeployCharacter(AppState* app, SessionState* session) {
    __time64_t timer;
    _time64(&timer);

    // Reset loading flags for this new phase
    // session->finished_loading = FALSE;
    // session->isReady = FALSE;

    printf("\n========== DEPLOY CHARACTER BEGIN (h1emu sequence) ==========\n");

    // h1emu sends AddSimpleNpc * many here for world objects.
    // We don't have world objects yet, so skip this.

    // 1. POIChangeMessage — h1emu sends this (we send empty/0)
    // Opcode 0x44 with no fields = just the opcode byte
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_POIChangeMessage, 0);

    // 2. Character.UpdateCharacterState — h1emu sends this
    Zone_Packet_Character_UpdateCharacterState charState = { 0 };
    charState.character_id = session->characterId;
    charState.game_time = timer & 0x7fffffff;
    // All state bytes default to 0
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &charState);

    // 3. DoneSendingPreloadCharacters — immediate
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // 4. DtoObjectInitialData — h1emu sends this (opcode unknown, skip for now)
    // TODO: find opcode and implement DtoObjectInitialData

    // 5. Character.CharacterStateDelta
    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    // 6. ZoneDoneSendingInitialData — IMMEDIATE (client needs this to exit loading)
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    // 7. Defer ONLY NetworkProximityUpdatesComplete — 5 seconds
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;
    session->characterReleased = TRUE;

    printf("[DEFER] NetworkProximityUpdatesComplete in ~5s\n");
    printf("========== DEPLOY CHARACTER END ==========\n\n");
}


void OnLogin(AppState* app, SessionState* session) {
    // ================================================================
    // h1emu sendInitData sequence (before ClientIsReady):
    //   1. InitializationParameters
    //   2. SendZoneDetails
    //   3. ClientGameSettings
    //   4. ReferenceData.DynamicAppearance (skin tones)
    //   5. SendSelfToClient
    //   6. Container.InitEquippedContainers
    //   7. (raw cached data: full DynamicAppearance + WeaponDefinitions)
    // ================================================================

    // 1. InitializationParameters
    Zone_Packet_InitializationParameters init_params = {
        .environment  = STR8("LIVE_KOTK"),
        .unk_string_1 = STR8("SKU_Is_KotK"),
        .ruleset_definitions_count = 0,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    // 2. SendZoneDetails
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

    // 3. ClientGameSettings
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

    // 4. ReferenceData.DynamicAppearance (skin tones — sent before SendSelfToClient)
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096, "data/ReferenceData_DynamicAppearance.bin");

    // 5. SendSelfToClient — THE ONLY TIME we send this (h1emu sends it once here)
    printf("[DEBUG] characterName: '%.*s' len=%d\n",
       (int)session->characterName.size,
       session->characterName.data,
       (int)session->characterName.size);
    printf("[DEBUG] characterId: 0x%llx\n", (unsigned long long)session->characterId);

    SendSelfToClient(app, session, FALSE);

    // 6. Container.InitEquippedContainers (empty)
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id = session->characterId;
    containers.container_list_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 7. Reference data — raw cached data that h1emu sends but doesn't log
    //    (these are sent as opaque blobs, the debug("send data") doesn't fire for them)
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 8192,  "data/Command_ItemDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 65536, "data/ReferenceData_WeaponDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096,  "data/ReferenceData_ProfileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 8192,  "data/ReferenceData_ProjectileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096,  "data/ReferenceData_ItemClassDefinitions.bin");

    // Reset loading flags before zone transition
    session->finished_loading = FALSE;
    session->isReady = FALSE;

    // ClientBeginZoning — triggers zone load
    // NOTE: h1emu comments this out for JS ("only necessary for transitioning between zones,
    // main menu is also a zone in KotK but not JS"). For KotK we still need it.
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