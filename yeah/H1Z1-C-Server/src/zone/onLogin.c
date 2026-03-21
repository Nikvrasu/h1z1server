void OnLogin(AppState* app, SessionState* session) {
    Zone_Packet_InitializationParameters init_params = {
        .environment = STR8("LIVE_KOTK"),
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    Zone_Packet_SendZoneDetails send_zone_details = {
        .zone_name = STR8("LoginZone"),
        .zone_type = 4,
        .unk_bool = FALSE,

        .overcast = 0,
        .fogDensity = 0,
        .fogFloor = 14.8f,
        .fogGradient = 15.25f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity = 0,
        .cloudWeight0 = 0.16f,
        .cloudWeight1 = 0.16f,
        .cloudWeight2 = 0.13f,
        .cloudWeight3 = 0.13f,
        .transitionTime = 0,
        .sunAxisX = 40,
        .sunAxisY = 0,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = 1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8(""),
        .stratusCloudTiling = 0.2f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight = 1000,
        .cumulusCloudTiling = 0.2f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness = 0.39f,
        .cloudSilverLiningBrightness = 0.5f,
        .cloudShadows = 0.2f,

        .zone_id = 5,
        .zone_id_2 = 5,
        .name_id = 7699,
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
        .overcast = 0,
        .fogDensity = 0,
        .fogFloor = 14.8f,
        .fogGradient = 15.25f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity = 0,
        .cloudWeight0 = 0.16f,
        .cloudWeight1 = 0.16f,
        .cloudWeight2 = 0.13f,
        .cloudWeight3 = 0.13f,
        .transitionTime = 0,
        .sunAxisX = 40,
        .sunAxisY = 0,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = 1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8(""),
        .stratusCloudTiling = 0.2f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight = 1000,
        .cumulusCloudTiling = 0.2f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness = 0.39f,
        .cloudSilverLiningBrightness = 0.5f,
        .cloudShadows = 0.2f,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_UpdateWeatherData,
                   &updt_weather_data);
    
    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -32.26f, .y = 506.41f, .z = 280.21f, .w = 1.f },
        .rotation = { .x = -0.11f, .y = -0.58f, .z = -0.08f, .w = 1.f },
        .trigger_loading_screen = FALSE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);
                
    printf("[DEBUG] characterName: '%.*s' len=%d\n", 
       (int)session->characterName.size, 
       session->characterName.data,
       (int)session->characterName.size);

    // EXPERIMENTAL!!!!
    // ClientBeginZoning — triggers zone load on the client
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
    beginZoning.wait_for_zone_ready          = TRUE;
    beginZoning.unk_bool_2                   = FALSE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientInitializationDetails, &initDetails);
    // EXPERIMENTAL ^
    // SendSelfToClient(app, session);
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 20000,
    "D:/h1z1server/yeah/H1Z1-C-Server/data/sendself_patched.bin");

    Zone_Packet_AddLightweightPc addPc = { 0 };
    addPc.character_id = session->characterId;
    addPc.transient_id.value = 52;
    addPc.id_characterFirstName = session->characterName;
    addPc.id_characterLastName = STR8("adad");
    addPc.id_unknownString1 = STR8("adad");
    addPc.id_characterName = session->characterName;
    addPc.actorModelId = 9240;
    addPc.position.x = -32.26f;
    addPc.position.y = 506.41f;
    addPc.position.z = 280.21f;
    addPc.rotation.x = -0.11f;
    addPc.rotation.y = -0.58f;
    addPc.rotation.z = -0.08f;
    addPc.rotation.w = 1.f;
    addPc.movementVersion = 1;
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_AddLightweightPc, &addPc);

    // Signal to client that initial zone data is complete.
    // Client waits for ZoneDoneSendingInitialData before sending
    // ClientInitializationDetails, SetLocale, and eventually ClientIsReady.
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    ZonePacketRawFileSend(app, session, &app->arenaPerTick, 256,
        "D:/h1z1server/yeah/H1Z1-C-Server/data/deploy.bin");
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_NetworkProximityUpdatesComplete, 0);
}