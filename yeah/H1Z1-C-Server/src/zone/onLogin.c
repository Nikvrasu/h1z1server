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
// ============================================================================
void SendEquipmentAndMovement(AppState* app, SessionState* session) {
    __time64_t eqTime;
    _time64(&eqTime);
    static int eqCount = 0;
    eqCount++;
    PRINT_TIMESTAMP(); printf("========== SEND EQUIPMENT & MOVEMENT (ClientFinishedLoading) ==========\n");
    printf("[EQUIP] SendEquipmentAndMovement called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d characterDeployed=%d\n",
           eqCount, eqTime, (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased,
           session->characterDeployed);

    // 1. GameTimeSync
    Zone_Packet_GameTimeSync gameTimeSync = { 0 };
    gameTimeSync.cycle_speed = 0.0f;
    gameTimeSync.time = 300000;
    gameTimeSync.unk_bool = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_GameTimeSync, &gameTimeSync);

    // 2. UpdateWeatherData
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

    // 3. Character.WeaponStance
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 4. Equipment.SetCharacterEquipment
    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1;

    String8 eqHeadActor = session->pGetPlayerActor.headActor;
    if (eqHeadActor.size == 0) eqHeadActor = STR8("SurvivorMale_Head_01.adr");

    String8 eqChestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 eqLegsModel = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
    String8 eqEyesModel = (gender == 2) ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");

    // Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    // setEquipment.unk_string_1 = STR8("Default");
    // setEquipment.unk_string_2 = STR8("#");
    // setEquipment.unk_bool_2 = TRUE;
    // setEquipment.length_1 = (struct length_1_s[1]){[0] = {
    //     .character_id = session->characterId, 
    //     .profile_id = 3,
    // }};
    // setEquipment.equipment_slot_array_count = 5;
    // setEquipment.equipment_slot_array = (struct equipment_slot_array_s[5]){
    //     [0] = { .equipment_slot_id_1 = 1, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 1, .guid = 0x1003, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    //     [1] = { .equipment_slot_id_1 = 3, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3, .guid = 0x1001, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    //     [2] = { .equipment_slot_id_1 = 4, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4, .guid = 0x1002, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    //     [3] = { .equipment_slot_id_1 = 7, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7, .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    //     [4] = { .equipment_slot_id_1 = 105, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 105, .guid = 0x1004, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    // };
    // setEquipment.attachments_data_1_count = 5;
    // setEquipment.attachments_data_1 = (struct attachments_data_1_s[5]){
    //     [0] = { .model_name = eqHeadActor, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1 },
    //     [1] = { .model_name = eqChestModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3 },
    //     [2] = { .model_name = eqLegsModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4 },
    //     [3] = { .model_name = STR8("Weapon_Empty.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7 },
    //     [4] = { .model_name = eqEyesModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105 },
    // };
    // ZonePacketSend(app, session, &app->arenaPerTick,
    //             Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);

    // 5. Loadout.SetLoadoutSlots
    Zone_Packet_Loadout_SetLoadoutSlots loadoutSlots = { 0 };
    loadoutSlots.character_id = session->characterId;
    loadoutSlots.loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    loadoutSlots.loadout_slot_data_count = 2;
    loadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[2]){
        [0] = {
            .hotbar_slot_id = LOADOUT_SLOT_MELEE,
            .loadout_id_1 = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id = LOADOUT_SLOT_MELEE,
            .item_def_id1 = WEAPON_FISTS,
            .loadout_item_guid = ITEM_GUID_FISTS,
            .unk_byte_1 = 0,
            .unk_dword_1 = 0,
        },
        [1] = {
            .hotbar_slot_id = LOADOUT_SLOT_BINOCULARS,
            .loadout_id_1 = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id = LOADOUT_SLOT_BINOCULARS,
            .item_def_id1 = WEAPON_BINOCULARS,
            .loadout_item_guid = ITEM_GUID_BINOCULARS,
            .unk_byte_1 = 0,
            .unk_dword_1 = 0,
        },
    };
    loadoutSlots.current_slot_id = LOADOUT_SLOT_MELEE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Loadout_SetLoadoutSlots, &loadoutSlots);

    // 6. Command.RunSpeed
    Zone_Packet_Command_RunSpeed runSpeed = { .run_speed = 7.5f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Command_RunSpeed, &runSpeed);

    // 7. ClientUpdate.ModifyMovementSpeed
    Zone_Packet_ClientUpdate_ModifyMovementSpeed moveSpeed = { 0 };
    moveSpeed.speed = 2.0f;
    moveSpeed.movementVersion = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ModifyMovementSpeed, &moveSpeed);

    printf("========== SEND EQUIPMENT & MOVEMENT END ==========\n\n");
}


void DeployCharacter(AppState* app, SessionState* session) {
    __time64_t timer;
    __time64_t deployTime;
    _time64(&timer);
    _time64(&deployTime);
    static int deployCount = 0;
    deployCount++;
    printf("[DEPLOY] DeployCharacter called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d characterDeployed=%d\n",
           deployCount, deployTime, (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased,
           session->characterDeployed);
    printf("[DEPLOY] Session actor: model=%u gender=%u head=%u hair='%.*s' headActor='%.*s'\n",
           session->pGetPlayerActor.actorModelId, session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType,
           (int)session->pGetPlayerActor.hairModel.size, session->pGetPlayerActor.hairModel.data,
           (int)session->pGetPlayerActor.headActor.size, session->pGetPlayerActor.headActor.data);

    PRINT_TIMESTAMP(); printf("========== DEPLOY CHARACTER BEGIN ==========\n");

    // 1. POIChangeMessage
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_POIChangeMessage, 0);

    // 2. Character.UpdateCharacterState
    Zone_Packet_Character_UpdateCharacterState charState = { 0 };
    charState.character_id = session->characterId;
    charState.state1 = 1;
    charState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &charState);

    // 3. DoneSendingPreloadCharacters
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // 4. DtoObjectInitialData
    {
        u8 dtoData[] = {
            0x05, 0x03,
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(dtoData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, dtoData, sizeof(dtoData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(dtoData) + TunnelDataHeaderLen);
        printf("[DEPLOY] Sent DtoObjectInitialData (raw 0x0503)\n");
    }

    session->characterReleased = TRUE;
    session->characterDeployed = TRUE;

    // 5. Character.CharacterStateDelta
    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    // 6. ZoneDoneSendingInitialData
    {
        __time64_t zdSendTime;
        _time64(&zdSendTime);
        printf("[TIMING] ZoneDoneSendingInitialData sent at %lld\n", zdSendTime);
    }
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    // 7. AccountItemManagerStateChanged
    {
        u8 escrowData[] = {
            0x23, 0x00,
            0x00,
            0x01,
            0x01,
            0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(escrowData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, escrowData, sizeof(escrowData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(escrowData) + TunnelDataHeaderLen);
        printf("[DEPLOY] Sent AccountItemManagerStateChanged (raw escrow packet)\n");
    }

    // 8. WeaponStance
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 9. Equipment.SetCharacterEquipment — triggers 3P composite model rebuild
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
        [0] = { .equipment_slot_id_1 = 3, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3, .guid = ITEM_GUID_FISTS,      .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 4, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4, .guid = ITEM_GUID_BINOCULARS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [2] = { .equipment_slot_id_1 = 7, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7, .guid = ITEM_GUID_FISTS,      .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    };
    setEquipment.attachments_data_1_count = 3;
    setEquipment.attachments_data_1 = (struct attachments_data_1_s[3]){
        [0] = { .model_name = STR8("SurvivorMale_Chest_Bra.adr"),              .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3 },
        [1] = { .model_name = STR8("SurvivorMale_Legs_Pants_Underwear.adr"),   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4 },
        [2] = { .model_name = STR8("Weapon_Empty.adr"),                        .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7 },
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);
    printf("[DEPLOY] Sent Equipment.SetCharacterEquipment (3P model rebuild trigger)\n");

    // 10. Deferred NetworkProximityUpdatesComplete
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;

    printf("[TIMER_DEFER] NetworkProximityUpdatesComplete scheduled for +5s\n");
    printf("========== DEPLOY CHARACTER END ==========\n\n");
}


void OnLogin(AppState* app, SessionState* session) {
    __time64_t onLoginTime;
    _time64(&onLoginTime);
    static int onLoginCount = 0;
    onLoginCount++;
    printf("[ONLOGIN] OnLogin called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx\n",
           onLoginCount, onLoginTime, (unsigned long long)session->characterId);

    PRINT_TIMESTAMP(); printf("[*] [TIMING] OnLogin BEGIN: 1. InitializationParameters\n");
    // 1. InitializationParameters
    Zone_Packet_InitializationParameters init_params = {
        .environment  = STR8("LIVE_KOTK"),
        .unk_string_1 = STR8("SKU_Is_KotK"),
        .ruleset_definitions_count = 0,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    PRINT_TIMESTAMP(); printf("[*] [TIMING] 2. SendZoneDetails\n");
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


    // 4. ReferenceData.DynamicAppearance — empty valid packet
    {
        u8 emptyDynAppearance[] = {
            0x17, 0x06,
            0x04, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, emptyDynAppearance, sizeof(emptyDynAppearance));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
    }

    // 5. SendSelfToClient
    printf("[DEBUG] characterName: '%.*s' len=%d\n",
       (int)session->characterName.size,
       session->characterName.data,
       (int)session->characterName.size);
    printf("[DEBUG] characterId: 0x%llx\n", (unsigned long long)session->characterId);

    SendSelfToClient(app, session, FALSE);

    Zone_Packet_AddLightweightPc lightweightPc = { 0 };
    lightweightPc.character_id = session->characterId;
    lightweightPc.transient_id.value = 52;
    lightweightPc.id_characterFirstName = session->characterName;
    lightweightPc.id_characterLastName = STR8("");
    lightweightPc.id_unknownString1 = STR8("00000000000000000");
    lightweightPc.id_characterName = session->characterName;
    lightweightPc.actorModelId = session->pGetPlayerActor.actorModelId;
    lightweightPc.position = (vec3){
        .x = -297.31f, .y = 506.06f, .z = -4894.10f
    };
    lightweightPc.rotation = (vec4){
        .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f
    };
    lightweightPc.movementVersion = 1;
    lightweightPc.flags1 = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_AddLightweightPc, &lightweightPc);

    Zone_Packet_Character_UpdateScale updateScale = { 0 };
    updateScale.character_id = session->characterId;
    updateScale.scale = (vec4){ .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f };
    ZonePacketSend(app, session, &app->arenaPerTick,
    Zone_Packet_Kind_Character_UpdateScale, &updateScale);

    // 5.5 Send equipment immediately so client has attachments before zone load
    {
        u32 eqGender = session->pGetPlayerActor.gender;
        if (eqGender == 0) eqGender = 1;

        String8 eqHeadActor = session->pGetPlayerActor.headActor;
        if (eqHeadActor.size == 0) eqHeadActor = (eqGender == 2) ? STR8("SurvivorFemale_Head_01.adr") : STR8("SurvivorMale_Head_01.adr");

        String8 eqChestModel = (eqGender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr")             : STR8("SurvivorMale_Chest_Bra.adr");
        String8 eqLegsModel  = (eqGender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr")  : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
        String8 eqEyesModel  = (eqGender == 2) ? STR8("SurvivorFemale_Eyes_01.adr")               : STR8("SurvivorMale_Eyes_01.adr");
        Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
        setEquipment.unk_string_1 = STR8("Default");
        setEquipment.unk_string_2 = STR8("#");
        setEquipment.unk_bool_2 = TRUE;
        setEquipment.length_1 = (struct length_1_s[1]){[0] = {
            .character_id = session->characterId,
            .profile_id = 3,
        }};
        setEquipment.equipment_slot_array_count = 5;
        setEquipment.equipment_slot_array = (struct equipment_slot_array_s[5]){
            [0] = { .equipment_slot_id_1 = 1, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 1, .guid = 0x1003, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
            [1] = { .equipment_slot_id_1 = 3, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3, .guid = 0x1001, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
            [2] = { .equipment_slot_id_1 = 4, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4, .guid = 0x1002, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
            [3] = { .equipment_slot_id_1 = 7, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7, .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
            [4] = { .equipment_slot_id_1 = 105, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 105, .guid = 0x1004, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        };
        setEquipment.attachments_data_1_count = 5;
        setEquipment.attachments_data_1 = (struct attachments_data_1_s[5]){
            [0] = { .model_name = eqHeadActor, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1 },
            [1] = { .model_name = eqChestModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3 },
            [2] = { .model_name = eqLegsModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4 },
            [3] = { .model_name = STR8("Weapon_Empty.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7 },
            [4] = { .model_name = eqEyesModel, .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105 },
        };
        ZonePacketSend(app, session, &app->arenaPerTick,
                       Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);
    }

    // 6. Container.InitEquippedContainers (empty)
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id = session->characterId;
    containers.container_list_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 7. Reference data — empty valid packets
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(10), "..\\data\\Command_ItemDefinitions.bin");

    {
        u8 emptyWeaponDefs[] = {
            0x17, 0x04,
            0x18, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyWeaponDefs) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, emptyWeaponDefs, sizeof(emptyWeaponDefs));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyWeaponDefs) + TunnelDataHeaderLen);
    }

    // Reset loading flags before zone transition
    session->finished_loading = FALSE;
    session->isReady = FALSE;

    __time64_t tzStart; _time64(&tzStart);
    printf("[ONLOGIN] Flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
    printf("[ONLOGIN] Session actorModelId=%u gender=%u headType=%u headActor='%.*s'\n",
           session->pGetPlayerActor.actorModelId, session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType,
           (int)session->pGetPlayerActor.headActor.size, session->pGetPlayerActor.headActor.data);

    // 8. ClientBeginZoning
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

    // 9. UpdateLocation
    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = TRUE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

    // 10. ClientInitializationDetails
    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                Zone_Packet_Kind_ClientInitializationDetails, &initDetails);

    __time64_t tzEnd; _time64(&tzEnd);
    printf("[ONLOGIN] All init packets sent in %lld seconds, waiting for ClientIsReady\n", tzEnd - tzStart);
    printf("[ONLOGIN] Post-init flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
}