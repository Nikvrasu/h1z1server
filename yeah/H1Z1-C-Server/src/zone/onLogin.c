// ============================================================================
// Full character deployment sequence. Sends everything the client needs
// to create ProxiedCharacter and transition out of WaitForZoneLoad.
// Called from OnLogin (initial) and from the 0x11 0x97 handler (post-zone-load).
// ============================================================================
void DeployCharacterPhase2(AppState* app, SessionState* session) {
    __time64_t timer;
    _time64(&timer);

    printf("[DEPLOY] Phase 2 begin for 0x%llx\n", (unsigned long long)session->characterId);

    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.ignore_this = 0;
    containers.character_id = session->characterId;
    containers.container_list_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    setEquipment.unk_string_1 = STR8("Default");
    setEquipment.unk_string_2 = STR8("#");
    setEquipment.unk_bool_2 = TRUE;
    setEquipment.length_1 = (struct length_1_s[1]){
        [0] = {
            .character_id = session->characterId,
            .profile_id = 5,
        },
    };
    setEquipment.equipment_slot_array_count = 0;
    setEquipment.attachments_data_1_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);

    Zone_Packet_Loadout_SetLoadoutSlots setLoadoutSlots = { 0 };
    setLoadoutSlots.character_id = session->characterId;
    setLoadoutSlots.loadout_id = 3;
    setLoadoutSlots.loadout_slot_data_count = 0;
    setLoadoutSlots.current_slot_id = 7;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Loadout_SetLoadoutSlots, &setLoadoutSlots);

    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_2 = 0x00ull;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.guid_4 = 0x00ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    Zone_Packet_GameTimeSync gameTimeSync = { 0 };
    gameTimeSync.cycle_speed = 12.f;
    gameTimeSync.time = timer;
    gameTimeSync.unk_bool = FALSE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_GameTimeSync, &gameTimeSync);

    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_NetworkProximityUpdatesComplete, 0);

    printf("[DEPLOY] Phase 2 complete\n");
}

void DeployCharacter(AppState* app, SessionState* session) {
    printf("[DEPLOY] Phase 1 begin for 0x%llx\n", (unsigned long long)session->characterId);

    SendSelfToClient(app, session);

    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(8),
        "D:/h1z1server/yeah/H1Z1-C-Server/data/Command.ItemDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(48),
        "D:/h1z1server/yeah/H1Z1-C-Server/data/ReferenceData.WeaponDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(8),
        "D:/h1z1server/yeah/H1Z1-C-Server/data/ReferenceData.ProjectileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(2),
        "D:/h1z1server/yeah/H1Z1-C-Server/data/ReferenceData.ProfileDefinitions.bin");
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(6),
        "D:/h1z1server/yeah/H1Z1-C-Server/data/ReferenceData.ItemClassDefinitions.bin");

    ZonePacketQueueLargeFile(app, session,
        "D:/h1z1server/yeah/H1Z1-C-Server/data/ReferenceData.DynamicAppearance.bin", 100);
    session->pendingPhase2 = TRUE;

    printf("[DEPLOY] Phase 1 complete, DynamicAppearance queued\n");
}

void OnLogin(AppState* app, SessionState* session) {
    Zone_Packet_InitializationParameters init_params = {
        .environment = STR8("LIVE_KOTK"),
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
        .zone_id_2 = 0,
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

    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = FALSE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

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
    beginZoning.zone_id_2                    = 0;
    beginZoning.name_id                      = 61609;
    beginZoning.unk_dword_1                  = 0x0f2b07d0;
    beginZoning.unk_bool_1                   = FALSE;
    beginZoning.wait_for_zone_ready          = FALSE;
    beginZoning.unk_bool_2                   = FALSE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientInitializationDetails, &initDetails);

    // Full character deployment after zone context is established
    DeployCharacter(app, session);

    Zone_Packet_AddLightweightPc addPc = { 0 };
    addPc.character_id = session->characterId;
    addPc.transient_id.value = 52;
    addPc.id_characterFirstName = session->characterName;
    addPc.id_characterLastName = STR8("");
    addPc.id_unknownString1 = STR8("");
    addPc.id_characterName = session->characterName;
    addPc.actorModelId = 9240;
    addPc.position.x = -297.31f;
    addPc.position.y = 506.06f;
    addPc.position.z = -4894.10f;
    addPc.rotation.x = 0.0f;
    addPc.rotation.y = -0.7071f;
    addPc.rotation.z = 0.0f;
    addPc.rotation.w = 0.7071f;
    addPc.movementVersion = 1;
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_AddLightweightPc, &addPc);

    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 64,
        "D:/h1z1server/yeah/H1Z1-C-Server/data/broadcast.bin");
}