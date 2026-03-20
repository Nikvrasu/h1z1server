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
       
    SendSelfToClient(app, session);

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
}