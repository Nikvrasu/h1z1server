// ============================================================================
// Hex dump helper
// ============================================================================
void HexDumpBuffer(const char* label, u8* data, u32 len) {
    printf("\n[HEX DUMP] %s (%u bytes):\n", label, len);
    for (u32 i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
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

    // 4. Equipment.SetCharacterEquipment — profile_id=3, unk_bool_2=FALSE (initial set not update)
    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1;

    String8 eqHeadActor = session->pGetPlayerActor.headActor;
    if (eqHeadActor.size == 0) eqHeadActor = STR8("SurvivorMale_Head_01.adr");

    String8 eqChestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 eqLegsModel  = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
    String8 eqEyesModel  = (gender == 2) ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");

    Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    setEquipment.unk_string_1 = STR8("Default");
    setEquipment.unk_string_2 = STR8("#");
    setEquipment.unk_bool_2 = FALSE;
    setEquipment.length_1 = (struct length_1_s[1]){[0] = {
        .character_id = session->characterId,
        .profile_id = 3,
    }};
    setEquipment.equipment_slot_array_count = 5;
    setEquipment.equipment_slot_array = (struct equipment_slot_array_s[5]){
        [0] = { .equipment_slot_id_1 = 1,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 1,   .guid =      0,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 3,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3,   .guid = 0x1001,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [2] = { .equipment_slot_id_1 = 4,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4,   .guid = 0x1002,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [3] = { .equipment_slot_id_1 = 7,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7,   .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [4] = { .equipment_slot_id_1 = 105, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 105, .guid = 0x1004,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    };
    setEquipment.attachments_data_1_count = 5;
    setEquipment.attachments_data_1 = (struct attachments_data_1_s[5]){
        [0] = { .model_name = eqHeadActor,                .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1   },
        [1] = { .model_name = eqChestModel,               .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3   },
        [2] = { .model_name = eqLegsModel,                .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4   },
        [3] = { .model_name = STR8("Weapon_Empty.adr"),   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7   },
        [4] = { .model_name = eqEyesModel,                .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105 },
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);
    printf("[EQUIP] Sent Equipment.SetCharacterEquipment (5 slots, profile_id=3, unk_bool_2=FALSE)\n");

    // 5. ClientUpdate.ActivateProfile — profile_id=3, full attachment list, actor_model_id set
    Zone_Packet_ClientUpdate_ActivateProfile activateProfile = { 0 };
    activateProfile.profile_payload = (struct profile_payload_s[1]){
        [0] = {
            .profile_id   = 3,
            .name_id      = 0,
            .desc_id      = 0,
            .type         = 3,
            .unk_dword_1  = 0,
            .ability_bg_image_set = 0,
            .badge_image_set      = 0,
            .button_image_set     = 0,
            .unk_byte_1   = 0,
            .unk_byte_2   = 0,
            .unk_dword_2  = 0,
            .unk_list_1_count = 0,
            .unk_dword_6  = 0,
            .unk_dword_7  = 0,
            .unk_byte_3   = 0,
            .unk_float_1  = 1.7f,
            .unk_float_2  = 0.95f,
            .unk_float_3  = 0.0f,
            .unk_dword_8  = 0,
            .unk_float_4  = 0.0f,
            .unk_dword_9  = 0,
            .unk_dword_10 = 0,
            .unk_dword_11 = 0,
            .unk_dword_12 = 0,
            .unk_dword_13 = 0,
        },
    };
    activateProfile.attachment_list_count = 5;
    activateProfile.attachment_list = (struct attachment_list_s[5]){
        [0] = { .model_name = eqHeadActor,              .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1,   .unk_bool_1 = FALSE },
        [1] = { .model_name = eqChestModel,             .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3,   .unk_bool_1 = FALSE },
        [2] = { .model_name = eqLegsModel,              .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4,   .unk_bool_1 = FALSE },
        [3] = { .model_name = STR8("Weapon_Empty.adr"), .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7,   .unk_bool_1 = FALSE },
        [4] = { .model_name = eqEyesModel,              .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105, .unk_bool_1 = FALSE },
    };
    activateProfile.unk_dword_1    = 0;
    activateProfile.unk_dword_2    = 0;
    activateProfile.actor_model_id = session->pGetPlayerActor.actorModelId;
    activateProfile.tint_alias     = STR8("Default");
    activateProfile.decal_alias    = STR8("#");
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ActivateProfile, &activateProfile);
    printf("[EQUIP] Sent ActivateProfile (profile_id=3, 5 attachments, actor_model_id=%u)\n",
           session->pGetPlayerActor.actorModelId);

    // 6. Loadout.SetLoadoutSlots
    Zone_Packet_Loadout_SetLoadoutSlots loadoutSlots = { 0 };
    loadoutSlots.character_id = session->characterId;
    loadoutSlots.loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    loadoutSlots.loadout_slot_data_count = 2;
    loadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[2]){
        [0] = {
            .hotbar_slot_id = LOADOUT_SLOT_MELEE,
            .loadout_id_1   = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id        = LOADOUT_SLOT_MELEE,
            .item_def_id1   = WEAPON_FISTS,
            .loadout_item_guid = ITEM_GUID_FISTS,
            .unk_byte_1     = 0,
            .unk_dword_1    = 0,
        },
        [1] = {
            .hotbar_slot_id = LOADOUT_SLOT_BINOCULARS,
            .loadout_id_1   = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id        = LOADOUT_SLOT_BINOCULARS,
            .item_def_id1   = WEAPON_BINOCULARS,
            .loadout_item_guid = ITEM_GUID_BINOCULARS,
            .unk_byte_1     = 0,
            .unk_dword_1    = 0,
        },
    };
    loadoutSlots.current_slot_id = LOADOUT_SLOT_MELEE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Loadout_SetLoadoutSlots, &loadoutSlots);

    // 7. Command.RunSpeed
    Zone_Packet_Command_RunSpeed runSpeed = { .run_speed = 7.5f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Command_RunSpeed, &runSpeed);

    // 8. ClientUpdate.ModifyMovementSpeed
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

    // 2. Character.UpdateCharacterState — show fully visible
    Zone_Packet_Character_UpdateCharacterState showState = { 0 };
    showState.character_id = session->characterId;
    showState.state1 = 1;
    showState.state2 = 1;
    showState.state3 = 1;
    showState.state4 = 1;
    showState.state5 = 1;
    showState.state6 = 1;
    showState.state7 = 1;
    showState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &showState);
    printf("[STATE] Sent CharacterState: all visible (state1-7=1)\n");

    // 3. DoneSendingPreloadCharacters
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // UpdateCamera — no-field trigger packet
    u8 updateCamera[] = { 0x57 };
    u8* camBuf = arena_push_size(&app->arenaPerTick, sizeof(updateCamera) + TunnelDataHeaderLen);
    memcpy(camBuf + TunnelDataHeaderLen, updateCamera, sizeof(updateCamera));
    GatewayTunnelDataSend(app, session, camBuf, sizeof(updateCamera) + TunnelDataHeaderLen);

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

    // Schedule UpdateCamera (0x57) to be sent 500ms later
    session->needsUpdateCamera = 1;
    session->updateCameraTick = *app->tickCount + 500;
    
    // ResourceEventBase — set character resources (case 0: set_character_resources_1)
    {
        Zone_Packet_ResourceEventBase resourceEvent = { 0 };
        resourceEvent.gametime = timer & 0x7fffffff;
        resourceEvent.variabletype8_case = 0;
        resourceEvent.variabletype8.set_character_resources_1.character_id_1 = session->characterId;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1_count = 9;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1 = (struct character_resources_1_s[9]){
            [0] = { .resource_type_1 = HEALTHTYPE,    .resource_id_1 = HEALTHID,    .resource_type_2 = HEALTHTYPE,    .value = 10000 },
            [1] = { .resource_type_1 = HUNGERTYPE,    .resource_id_1 = HUNGERID,    .resource_type_2 = HUNGERTYPE,    .value = 10000 },
            [2] = { .resource_type_1 = HYDRATIONTYPE, .resource_id_1 = HYDRATIONID, .resource_type_2 = HYDRATIONTYPE, .value = 10000 },
            [3] = { .resource_type_1 = STAMINATYPE,   .resource_id_1 = STAMINAID,   .resource_type_2 = STAMINATYPE,   .value = 10000 },
            [4] = { .resource_type_1 = VIRUSTYPE,     .resource_id_1 = VIRUSID,     .resource_type_2 = VIRUSTYPE,     .value = 0     },
            [5] = { .resource_type_1 = BLEEDINGTYPE,  .resource_id_1 = BLEEDINGID,  .resource_type_2 = BLEEDINGTYPE,  .value = 0     },
            [6] = { .resource_type_1 = COMFORTTYPE,   .resource_id_1 = COMFORTID,   .resource_type_2 = COMFORTTYPE,   .value = 5000  },
            [7] = { .resource_type_1 = FUELTYPE,      .resource_id_1 = FUELID,      .resource_type_2 = FUELTYPE,      .value = 0     },
            [8] = { .resource_type_1 = CONDITIONTYPE, .resource_id_1 = CONDITIONID, .resource_type_2 = CONDITIONTYPE, .value = 10000 },
        };
        ZonePacketSend(app, session, &app->arenaPerTick,
                    Zone_Packet_Kind_ResourceEventBase, &resourceEvent);
    }

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

    // NOTE: No Equipment.SetCharacterEquipment here.
    // Equipment is sent in SendEquipmentAndMovement on ClientFinishedLoading,
    // after zone assets are fully loaded in memory.

    // 9. Deferred NetworkProximityUpdatesComplete
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;

    printf("[TIMER_DEFER] NetworkProximityUpdatesComplete scheduled for +5s\n");
    printf("========== DEPLOY CHARACTER END ==========\n\n");
}


void OnLogin(AppState* app, SessionState* session) {
    __time64_t onLoginTime;
    _time64(&onLoginTime);
    __time64_t timer;
    _time64(&timer);
    static int onLoginCount = 0;
    onLoginCount++;
    printf("[ONLOGIN] OnLogin called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx\n",
           onLoginCount, onLoginTime, (unsigned long long)session->characterId);

    // FIRST: ensure actor data has valid defaults before any packet uses it.
    // pGetPlayerActor is populated by the login server's GetHeadTypeId, but
    // the zone server session starts fresh. If the data didn't transfer, default to male head 1.
    if (session->pGetPlayerActor.actorModelId == 0) {
        session->pGetPlayerActor.actorModelId = 9469;
        session->pGetPlayerActor.gender       = 1;
        session->pGetPlayerActor.headType     = 1;
        session->pGetPlayerActor.headActor    = STR8("SurvivorMale_Head_01.adr");
        session->pGetPlayerActor.hairModel    = STR8("SurvivorMale_Hair_MediumMessy.adr");
        printf("[ONLOGIN] Actor data was empty — applied male head 1 defaults\n");
    }

    PRINT_TIMESTAMP(); printf("[*] [TIMING] OnLogin BEGIN: 0. Hide Character\n");

    // 0. Character.UpdateCharacterState — hide during loading (state1=0)
    Zone_Packet_Character_UpdateCharacterState hideState = { 0 };
    hideState.character_id = session->characterId;
    hideState.state1 = 0;
    hideState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &hideState);
    printf("[STATE] Sent CharacterState: hidden (state1=0)\n");

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
        .unk_bool  = FALSE,
        .overcast  = 1.0f,
        .fogDensity = 0.000173f,
        .fogFloor   = 10.0f,
        .fogGradient = 0.0144f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity  = 0,
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
        .stratusCloudTiling  = 0.30f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight  = 1000,
        .cumulusCloudTiling  = 0.20f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight  = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness  = 0.25f,
        .cloudSilverLiningBrightness = 7.0f,
        .cloudShadows = 0.5f,
        .zone_id   = 5,
        .zone_id_2 = 5,
        .name_id   = 61609,
        .unk_bool2 = TRUE,
        .lighting  = STR8("Lighting_Z2.txt"),
        .unk_bool3 = FALSE,
        .unk_bool4 = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendZoneDetails,
                   &send_zone_details);

    // 3. ClientGameSettings
    Zone_Packet_ClientGameSettings game_settings = {
        .interact_glow_and_dist = 16,
        .unk_bool         = TRUE,
        .timescale        = 1.0,
        .enable_weapons   = 1,
        .unk_u32_2        = 1,
        .unk_float2       = 15.,
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
    printf("[DEBUG] actorModelId: %u gender: %u headType: %u\n",
           session->pGetPlayerActor.actorModelId,
           session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType);

    SendSelfToClientRaw(app, session);

    // 6. AddLightweightPc — broadcast self presence to proximity system
    Zone_Packet_AddLightweightPc lightweightPc = { 0 };
    lightweightPc.character_id          = session->characterId;
    lightweightPc.transient_id.value    = 1;
    lightweightPc.id_characterFirstName = session->characterName;
    lightweightPc.id_characterLastName  = STR8("");
    lightweightPc.id_unknownString1     = STR8("00000000000000000");
    lightweightPc.id_characterName      = session->characterName;
    lightweightPc.actorModelId          = session->pGetPlayerActor.actorModelId;
    lightweightPc.position = (vec3){
        .x = -297.31f, .y = 506.06f, .z = -4894.10f
    };
    lightweightPc.rotation = (vec4){
        .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f
    };
    lightweightPc.movementVersion = 1;
    lightweightPc.flags1          = 0;
    ZonePacketSendDebug(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_AddLightweightPc, &lightweightPc, "AddLightweightPc");

    // 7. Character.UpdateScale
    Zone_Packet_Character_UpdateScale updateScale = { 0 };
    updateScale.character_id = session->characterId;
    updateScale.scale = (vec4){ .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateScale, &updateScale);

    // 8. Container.InitEquippedContainers (empty)
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id        = session->characterId;
    containers.container_list_count = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 9. Reference data
    //ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(10), "..\\data\\Command_ItemDefinitions.bin");
    // Command.ItemDefinitions — empty list, no server-side item defs
    {
        u8 emptyItemDefs[] = {
            0x09, 0x47, 0x00,       // opcode + sub-opcode (CommandItemDefinitions)
            0x08, 0x00, 0x00, 0x00, // stream length = 8 bytes
            0x00, 0x00, 0x00, 0x00, // list count = 0 item_defs
            0x00, 0x00, 0x00, 0x00, // (padding to match stream length)
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyItemDefs) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, emptyItemDefs, sizeof(emptyItemDefs));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyItemDefs) + TunnelDataHeaderLen);
    }
    Zone_Packet_ReferenceDataWeaponDefinitions weaponDefs = { 0 };

weaponDefs.weapon_byteswithlength_length = 1;
weaponDefs.weapon_byteswithlength = (struct weapon_byteswithlength_s[1]){
    [0] = {
        .weapon_defs_count = 1,
        .weapon_defs = (struct weapon_defs_s[1]){
            [0] = {
                .id1                 = 85,
                .id2                 = 85,
                .weapon_group_id     = 0,
                .flags1              = 0,
                .equip_ms            = 500,
                .unequip_ms          = 500,
                .melee_detect_width  = 100,
                .melee_detect_height = 100,
                .anim_set_name       = STR8("Fists"),
                .ammo_slots_count    = 0,
                .fire_groups_count   = 1,
                .fire_groups = (struct fire_groups_s[1]){
                    [0] = { .fire_group_id = 1 },
                },
            },
        },
        .fire_group_defs_count = 1,
        .fire_group_defs = (struct fire_group_defs_s[1]){
            [0] = {
                .id3 = 1,
                .id4 = 1,
                .fire_mode_list_count = 1,
                .fire_mode_list = (struct fire_mode_list_s[1]){
                    [0] = { .fire_mode_1 = 1 },
                },
            },
        },
        .fire_mode_defs_count = 1,
        .fire_mode_defs = (struct fire_mode_defs_s[1]){
            [0] = {
                .id5            = 1,
                .id6            = 1,
                .type           = 1,
                .refire_time_ms = 500,
                .range          = 2.0f,
                // Third person camera
                .tp_force_camera_overrides  = TRUE,
                .tp_camera_distance         = 3.5f,
                .tp_cr_camera_distance      = 3.0f,
                .tp_pr_camera_distance      = 3.5f,
                .tp_camera_fov              = 75.0f,
                .tp_cr_camera_fov           = 75.0f,
                .tp_pr_camera_fov           = 75.0f,
                .fp_force_camera_overrides  = FALSE,
                .fp_camera_fov              = 250.0f,
            },
        },
        .player_state_group_defs_count          = 0,
        .fire_mode_projectile_mapping_data_count = 0,
        .aim_assist_defs_count                  = 0,
    },
};

ZonePacketSend(app, session, &app->arenaPerTick,
               Zone_Packet_Kind_ReferenceDataWeaponDefinitions, &weaponDefs);

    // Reset loading flags before zone transition
    session->finished_loading = FALSE;
    session->isReady          = FALSE;

    printf("[ONLOGIN] Flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
    printf("[ONLOGIN] Session actorModelId=%u gender=%u headType=%u headActor='%.*s'\n",
           session->pGetPlayerActor.actorModelId, session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType,
           (int)session->pGetPlayerActor.headActor.size, session->pGetPlayerActor.headActor.data);

    // 10. ClientBeginZoning
    Zone_Packet_ClientBeginZoning beginZoning = { 0 };
    beginZoning.zone_name  = STR8("Z2");
    beginZoning.zone_type  = 4;
    beginZoning.pos        = (vec4){ .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f };
    beginZoning.rot        = (vec4){ .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f };
    beginZoning.overcast   = 1.0f;
    beginZoning.fogDensity = 0.000173f;
    beginZoning.fogFloor   = 10.0f;
    beginZoning.fogGradient = 0.0144f;
    beginZoning.globalPrecipitation = 0.0f;
    beginZoning.temperature = 75.0f;
    beginZoning.skyClarity  = 0.0f;
    beginZoning.cloudWeight0 = 0.05f;
    beginZoning.cloudWeight1 = 0.0f;
    beginZoning.cloudWeight2 = 0.05f;
    beginZoning.cloudWeight3 = 0.15f;
    beginZoning.transitionTime = 0.0f;
    beginZoning.sunAxisX = 38.0f;
    beginZoning.sunAxisY = -15.0f;
    beginZoning.sunAxisZ = 0.0f;
    beginZoning.windDirX = -1.0f;
    beginZoning.windDirY = -0.5f;
    beginZoning.windDirZ = -1.0f;
    beginZoning.wind = 3.0f;
    beginZoning.rainMinStrength       = 0.0f;
    beginZoning.rainRampUpTimeSeconds = 1.0f;
    beginZoning.cloudFile             = STR8("sky_Z_clouds.dds");
    beginZoning.stratusCloudTiling    = 0.30f;
    beginZoning.stratusCloudScrollU   = -0.002f;
    beginZoning.stratusCloudScrollV   = 0.0f;
    beginZoning.stratusCloudHeight    = 1000.0f;
    beginZoning.cumulusCloudTiling    = 0.20f;
    beginZoning.cumulusCloudScrollU   = 0.0f;
    beginZoning.cumulusCloudScrollV   = 0.002f;
    beginZoning.cumulusCloudHeight    = 8000.0f;
    beginZoning.cloudAnimationSpeed   = 0.0f;
    beginZoning.cloudSilverLiningThickness  = 0.25f;
    beginZoning.cloudSilverLiningBrightness = 7.0f;
    beginZoning.cloudShadows    = 0.5f;
    beginZoning.unk_byte_1      = 4;
    beginZoning.zone_id_1       = 5;
    beginZoning.zone_id_2       = 5;
    beginZoning.name_id         = 61609;
    beginZoning.unk_dword_1     = 0x0f2b07d0;
    beginZoning.unk_bool_1      = FALSE;
    beginZoning.wait_for_zone_ready = FALSE;
    beginZoning.unk_bool_2      = FALSE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

    // 11. UpdateLocation
    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = TRUE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

    // 12. ClientInitializationDetails
    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientInitializationDetails, &initDetails);

    __time64_t tzEnd; _time64(&tzEnd);
    printf("[ONLOGIN] All init packets sent in %lld seconds, waiting for ClientIsReady\n", tzEnd - onLoginTime);
    printf("[ONLOGIN] Post-init flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
}