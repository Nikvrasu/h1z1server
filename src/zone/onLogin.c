// ============================================================================
// Zone Server — OnLogin, DeployCharacter, SendEquipmentAndMovement
//
// This file implements the three-phase character initialization:
//   Phase 1: OnLogin        — SendInitData (called when gateway login succeeds)
//   Phase 2: DeployCharacter — ClientIsReady (client loaded zone, deploy entity)
//   Phase 3: SendEquipment  — ClientFinishedLoading (client ready for gameplay)
//
// Ref: H1emu/h1z1-server src/servers/ZoneServer2016/zoneserver.ts
//   - sendInitData() → OnLogin
//   - ClientIsReady handler → DeployCharacter
//   - ClientFinishedLoading handler → SendEquipmentAndMovement
// ============================================================================

// ============================================================================
// Debug Helpers
// ============================================================================
void HexDumpBuffer(const char* label, u8* data, u32 len) {
    printf("\n[HEX] %s (%u bytes):\n", label, len);
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
    printf(MESSAGE_CONCAT_INFO("DEBUG %s kind=%d len=%u\n"), label, kind, packedLen);
    HexDumpBuffer(label, packedBuffer, packedLen);
    u32 totalLen = packedLen + TunnelDataHeaderLen;
    GatewayTunnelDataSend(app, session, baseBuffer, totalLen);
}


// ============================================================================
// Phase 3: SendEquipmentAndMovement
//
// Called from DeployCharacter after entity is created in-world.
// Sends equipment, profile, loadout, and movement speed to the client.
//
// Ref: H1emu zoneserver.ts ClientFinishedLoading handler
//   1. GameTimeSync
//   2. UpdateWeatherData
//   3. Character.WeaponStance
//   4. Equipment.SetCharacterEquipment
//   5. ClientUpdate.ActivateProfile
//   6. Loadout.SetLoadoutSlots
//   7. Command.RunSpeed
//   8. ClientUpdate.ModifyMovementSpeed
// ============================================================================
void SendEquipmentAndMovement(AppState* app, SessionState* session) {
    printf(MESSAGE_CONCAT_INFO("SendEquipmentAndMovement for 0x%llx\n"),
           (unsigned long long)session->characterId);

    // Resolve gender-aware models
    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1;

    String8 eqHeadActor = session->pGetPlayerActor.headActor;
    if (eqHeadActor.size == 0) eqHeadActor = STR8("SurvivorMale_Head_01.adr");

    String8 eqChestModel = GetChestModel(gender);
    String8 eqLegsModel  = GetLegsModel(gender);
    String8 eqEyesModel  = GetEyesModel(gender);

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
    // Ref: H1emu character.ts pGetEquipment() — initial set, not update
    Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    setEquipment.unk_string_1 = STR8("Default");
    setEquipment.unk_string_2 = STR8("#");
    setEquipment.unk_bool_2 = FALSE;
    setEquipment.length_1 = (struct length_1_s[1]){[0] = {
        .character_id = session->characterId,
        .profile_id = 4,
    }};
    setEquipment.equipment_slot_array_count = 8;
    setEquipment.equipment_slot_array = (struct equipment_slot_array_s[8]){
        [0] = { .equipment_slot_id_1 = 1,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 1,   .guid =      0,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 3,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3,   .guid = 0x1001,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [2] = { .equipment_slot_id_1 = 4,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4,   .guid = 0x1002,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [3] = { .equipment_slot_id_1 = 7,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7,   .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [4] = { .equipment_slot_id_1 = 105, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 105, .guid = 0x1004,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [5] = { .equipment_slot_id_1 = 10,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 10,  .guid = 0x1005,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [6] = { .equipment_slot_id_1 = 14,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 14,  .guid = 0x1006,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [7] = { .equipment_slot_id_1 = 13,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 13,  .guid = 0x1007,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    };
    setEquipment.attachments_data_1_count = 8;
    setEquipment.attachments_data_1 = (struct attachments_data_1_s[8]){
        [0] = { .model_name = eqHeadActor,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1   },
        [1] = { .model_name = eqChestModel,                                   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3   },
        [2] = { .model_name = eqLegsModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4   },
        [3] = { .model_name = STR8("Weapon_Empty.adr"),                       .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7   },
        [4] = { .model_name = eqEyesModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105 },
        [5] = { .model_name = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),     .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 10  },
        [6] = { .model_name = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),  .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 14  },
        [7] = { .model_name = STR8("SurvivorMale_Feet_Conveys.adr"),          .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 13  },
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);

    // 5. ClientUpdate.ActivateProfile
    // Ref: H1emu character.ts pGetActivateProfile()
    Zone_Packet_ClientUpdate_ActivateProfile activateProfile = { 0 };
    activateProfile.profile_payload = (struct profile_payload_s[1]){
        [0] = {
            .profile_id   = 4,
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
    activateProfile.attachment_list_count = 8;
    activateProfile.attachment_list = (struct attachment_list_s[8]){
        [0] = { .model_name = eqHeadActor,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1,   .unk_bool_1 = FALSE },
        [1] = { .model_name = eqChestModel,                                   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3,   .unk_bool_1 = FALSE },
        [2] = { .model_name = eqLegsModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4,   .unk_bool_1 = FALSE },
        [3] = { .model_name = STR8("Weapon_Empty.adr"),                       .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7,   .unk_bool_1 = FALSE },
        [4] = { .model_name = eqEyesModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105, .unk_bool_1 = FALSE },
        [5] = { .model_name = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),     .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 10,  .unk_bool_1 = FALSE },
        [6] = { .model_name = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),  .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 14,  .unk_bool_1 = FALSE },
        [7] = { .model_name = STR8("SurvivorMale_Feet_Conveys.adr"),          .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 13,  .unk_bool_1 = FALSE },
    };
    activateProfile.unk_dword_1    = 0;
    activateProfile.unk_dword_2    = 0;
    activateProfile.actor_model_id = session->pGetPlayerActor.actorModelId;
    activateProfile.tint_alias     = STR8("Default");
    activateProfile.decal_alias    = STR8("#");
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ActivateProfile, &activateProfile);
    printf("[EQUIP] Sent ActivateProfile (profile_id=3, 8 attachments, actor_model_id=%u)\n",
           session->pGetPlayerActor.actorModelId);

    // 6. Loadout.SetLoadoutSlots
    Zone_Packet_Loadout_SetLoadoutSlots loadoutSlots = { 0 };
    loadoutSlots.character_id = session->characterId;
    loadoutSlots.loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    loadoutSlots.loadout_slot_data_count = 5;
    loadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[5]){
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
        [2] = {
            .hotbar_slot_id    = 10,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 10,
            .item_def_id1      = 5747,
            .loadout_item_guid = 0x1005,
        },
        [3] = {
            .hotbar_slot_id    = 14,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 14,
            .item_def_id1      = 2178,
            .loadout_item_guid = 0x1006,
        },
        [4] = {
            .hotbar_slot_id    = 13,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 13,
            .item_def_id1      = 2216,
            .loadout_item_guid = 0x1007,
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
}


// ============================================================================
// Phase 2: DeployCharacter
//
// Called when client sends ClientIsReady (0x04) after loading the zone.
// Creates the character entity in-world and sends all deployment packets.
//
// Ref: H1emu zoneserver.ts onClientIsReady() handler
//   1. POIChangeMessage
//   2. Character.UpdateCharacterState (show visible)
//   3. DoneSendingPreloadCharacters
//   4. UpdateCamera
//   5. DtoObjectInitialData
//   6. Character.CharacterStateDelta
//   7. AddLightweightPc (register self as world entity)
//   8. SendEquipmentAndMovement (equipment, profile, loadout)
//   9. LightweightToFullPc (upgrade entity)
//  10. ZoneDoneSendingInitialData
//  11. ResourceEventBase (health, hunger, etc.)
//  12. AccountItemManagerStateChanged
//  13. Character.WeaponStance
//  14. Deferred NetworkProximityUpdatesComplete (+5s)
// ============================================================================
void DeployCharacter(AppState* app, SessionState* session) {
    __time64_t timer;
    _time64(&timer);

    printf(MESSAGE_CONCAT_INFO("DeployCharacter for 0x%llx (cycle=%u)\n"),
           (unsigned long long)session->characterId, session->zoneCycleId);

    // Guard: ensure valid cycle ID
    if (session->zoneCycleId == 0) {
        session->zoneCycleId = 1;
    }

    // Guard: deploy exactly once per zone cycle (prevents ClientIsReady race)
    if (session->deployedCycleId == session->zoneCycleId) {
        printf(MESSAGE_CONCAT_WARN("Duplicate deploy blocked for cycle %u\n"),
               session->zoneCycleId);
        return;
    }

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

    // 3. DoneSendingPreloadCharacters
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // UpdateCamera
    u8 updateCamera[] = { 0x57 };
    u8* camBuf = arena_push_size(&app->arenaPerTick, sizeof(updateCamera) + TunnelDataHeaderLen);
    memcpy(camBuf + TunnelDataHeaderLen, updateCamera, sizeof(updateCamera));
    GatewayTunnelDataSend(app, session, camBuf, sizeof(updateCamera) + TunnelDataHeaderLen);

    // 4. DtoObjectInitialData — empty DTO init
    // Ref: H1emu opcode 0xbb0300 DtoObjectInitialData
    {
        u8 dtoData[] = {
            0x05, 0x03,                     // opcode
            0x01, 0x00, 0x00, 0x00,         // unknownDword1
            0x00, 0x00, 0x00, 0x00,         // unknownArray1 count
            0x00, 0x00, 0x00, 0x00,         // unknownArray2 count
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(dtoData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, dtoData, sizeof(dtoData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(dtoData) + TunnelDataHeaderLen);
    }

    session->characterReleased = TRUE;
    session->characterDeployed = TRUE;
    session->deployedCycleId = session->zoneCycleId;

    // 5. Character.CharacterStateDelta
    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    // 5b. AddLightweightPc — register self as world entity
    // Ref: H1emu zoneserver.ts sendCharacterData() → AddLightweightPc
    Zone_Packet_AddLightweightPc lightweightPc = { 0 };
    lightweightPc.character_id       = session->characterId;
    lightweightPc.transient_id.value = session->zoneCycleId;
    lightweightPc.id_characterName   = session->characterName;
    lightweightPc.actorModelId       = session->pGetPlayerActor.actorModelId;
    lightweightPc.position           = (vec3){ -297.309998f, 506.059998f, -4894.100098f };
    lightweightPc.rotation           = (vec4){ 0.0f, -0.707100f, 0.0f, 0.707100f };
    lightweightPc.unknownFloat1      = 1.0f;
    lightweightPc.flags1             = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_AddLightweightPc, &lightweightPc);

    // 6. SendEquipmentAndMovement — BEFORE ZoneDone so ProcessNewAttachment fires
    //    while client is still in WaitForFirstZone, giving geometry time to load
    SendEquipmentAndMovement(app, session);

    // 7. LightweightToFullPc — upgrade entity with position/rotation data
    ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(2), "..\\data\\LightweightToFullPc.bin");

    // 8. ZoneDoneSendingInitialData
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    // 9. ResourceEventBase
    {
        Zone_Packet_ResourceEventBase resourceEvent = { 0 };
        resourceEvent.gametime = timer & 0x7fffffff;
        resourceEvent.variabletype8_case = 0;
        resourceEvent.variabletype8.set_character_resources_1.character_id_1 = session->characterId;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1_count = 4;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1 = (struct character_resources_1_s[4]){
            [0] = { .resource_type_1 = HEALTHTYPE,    .resource_id_1 = HEALTHID,    .resource_type_2 = HEALTHTYPE,    .value = 10000 },
            [1] = { .resource_type_1 = HUNGERTYPE,    .resource_id_1 = HUNGERID,    .resource_type_2 = HUNGERTYPE,    .value = 10000 },
            [2] = { .resource_type_1 = HYDRATIONTYPE, .resource_id_1 = HYDRATIONID, .resource_type_2 = HYDRATIONTYPE, .value = 10000 },
            [3] = { .resource_type_1 = STAMINATYPE,   .resource_id_1 = STAMINAID,   .resource_type_2 = STAMINATYPE,   .value = 10000 },
        };
        ZonePacketSend(app, session, &app->arenaPerTick,
                    Zone_Packet_Kind_ResourceEventBase, &resourceEvent);
    }

    // 10. AccountItemManagerStateChanged — escrow state init
    // Ref: H1emu zoneserver.ts sends this after resource init
    {
        u8 escrowData[] = {
            0x23, 0x00,     // opcode
            0x00,           // unkDword1
            0x01,           // state
            0x01,           // unkBool
            0x00,           // unkBool2
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(escrowData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, escrowData, sizeof(escrowData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(escrowData) + TunnelDataHeaderLen);
    }

    // 11. WeaponStance — initial stance (0 = idle)
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 12. Deferred NetworkProximityUpdatesComplete (+5s)
    // Ref: H1emu defers this to allow character data to propagate
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;
}


// ============================================================================
// Phase 1: OnLogin — sendInitData
//
// Called when the gateway login succeeds. Sends all initial zone data:
//   0. Character.UpdateCharacterState (hide during loading)
//   1. InitializationParameters
//   2. SendZoneDetails
//   3. ClientGameSettings
//   4. ReferenceData.DynamicAppearance
//   5. SendSelfToClient
//   6. Character.UpdateScale
//   7. Container.InitEquippedContainers
//   8. Command.ItemDefinitions
//   9. ReferenceData.WeaponDefinitions
//  10. ClientUpdate.UpdateLocation
//  11. ClientInitializationDetails
//
// Ref: H1emu zoneserver.ts sendInitData()
// ============================================================================
void OnLogin(AppState* app, SessionState* session) {
    __time64_t timer;
    _time64(&timer);

    printf(MESSAGE_CONCAT_INFO("OnLogin for 0x%llx '%.*s'\n"),
           (unsigned long long)session->characterId,
           (int)session->characterName.size, session->characterName.data);

    // Ensure actor data has valid defaults (populated by login server's
    // SetPlayerActorFromHeadType, but zone session may start fresh)
    if (session->pGetPlayerActor.actorModelId == 0) {
        // Inline default: male head 1
        session->pGetPlayerActor.actorModelId = 9469;
        session->pGetPlayerActor.gender       = 1;
        session->pGetPlayerActor.headType     = 1;
        session->pGetPlayerActor.headActor    = STR8("SurvivorMale_Head_01.adr");
        session->pGetPlayerActor.hairModel    = STR8("SurvivorMale_Hair_MediumMessy.adr");
        printf(MESSAGE_CONCAT_INFO("Actor data was empty — applied male head 1 defaults\n"));
    }

    // 0. Character.UpdateCharacterState — hide during loading
    Zone_Packet_Character_UpdateCharacterState hideState = { 0 };
    hideState.character_id = session->characterId;
    hideState.state1 = 0;
    hideState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &hideState);

    // 1. InitializationParameters
    Zone_Packet_InitializationParameters init_params = {
        .environment  = STR8("LIVE_KOTK"),
        .unk_string_1 = STR8("SKU_Is_KotK"),
        .ruleset_definitions_count = 0,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    // 2. SendZoneDetails — Z2 map with weather configuration
    // Ref: H1emu zoneserver.ts sendZoneDetails()
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
    // Ref: H1emu zoneserver.ts sendGameSettings()
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
            0x17, 0x06,                     // opcode
            0x04, 0x00, 0x00, 0x00,         // unknownDword1
            0x00, 0x00, 0x00, 0x00,         // array count
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, emptyDynAppearance, sizeof(emptyDynAppearance));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
    }

    // 5. SendSelfToClient — full character data packet
    // Ref: H1emu zoneserver.ts sendCharacterData() → SendSelfToClient
    SendSelfToClient(app, session, FALSE);

    // 6. Character.UpdateScale
    // Ref: H1emu zoneserver.ts sendInitData() scale=1.0
    Zone_Packet_Character_UpdateScale updateScale = { 0 };
    updateScale.character_id = session->characterId;
    updateScale.scale = (vec4){ .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateScale, &updateScale);

    // 7. Container.InitEquippedContainers
    // Ref: H1emu zoneserver.ts sendInitData() → Container.InitEquippedContainers
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id = session->characterId;
    containers.ignore_this  = 0;

    containers.container_list_count = 5;
    containers.container_list = (struct container_list_s[5]){
        [0] = {
            .loadout_slot_id        = 10,
            .guid_1                 = 0x1005,
            .defs_id                = 5747,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 5747,
                .item_defs_id_2     = 5747,
                .tint_id            = 0,
                .guid_2             = 0x1005,
                .count              = 1,
                .container_guid     = 0x1005,
                .contain_def_id     = 1,
                .container_slot_id  = 10,
                .base_durability    = 0,
                .current_durability = 0,
                .max_durability_from_defs = 0,
                .unk_bool_1         = FALSE,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
            .max_bulk       = 0,
            .bulk_used      = 0,
            .has_bulk_limit = FALSE,
        },
        [1] = {
            .loadout_slot_id        = 14,
            .guid_1                 = 0x1006,
            .defs_id                = 2178,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 2178,
                .item_defs_id_2     = 2178,
                .guid_2             = 0x1006,
                .count              = 1,
                .container_guid     = 0x1006,
                .contain_def_id     = 1,
                .container_slot_id  = 14,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [2] = {
            .loadout_slot_id        = 13,
            .guid_1                 = 0x1007,
            .defs_id                = 2216,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 2216,
                .item_defs_id_2     = 2216,
                .guid_2             = 0x1007,
                .count              = 1,
                .container_guid     = 0x1007,
                .contain_def_id     = 1,
                .container_slot_id  = 13,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [3] = {
            .loadout_slot_id        = 7,
            .guid_1                 = ITEM_GUID_FISTS,
            .defs_id                = 85,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 85,
                .item_defs_id_2     = 85,
                .guid_2             = ITEM_GUID_FISTS,
                .count              = 1,
                .container_guid     = ITEM_GUID_FISTS,
                .contain_def_id     = 1,
                .container_slot_id  = 7,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [4] = {
            .loadout_slot_id        = 5,
            .guid_1                 = ITEM_GUID_BINOCULARS,
            .defs_id                = 1542,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 1542,
                .item_defs_id_2     = 1542,
                .guid_2             = ITEM_GUID_BINOCULARS,
                .count              = 1,
                .container_guid     = ITEM_GUID_BINOCULARS,
                .contain_def_id     = 1,
                .container_slot_id  = 5,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
    };

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 8. Command.ItemDefinitions — 5 items: fists, binoculars, hoodie, jeans, sneakers
    // Ref: H1emu zoneserver.ts sendInitData() → Command.ItemDefinitions
    Zone_Packet_CommandItemDefinitions itemDefs = { 0 };

itemDefs.item_def_reply_2_length = 1;
itemDefs.item_def_reply_2 = (struct item_def_reply_2_s[1]){
    [0] = {
        .item_defs_count = 5,
        .item_defs = (struct item_defs_s[5]){
            [0] = {
                // Fists
                .defs_id       = 85,
                .bitflags1     = 0,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP | FLAG_NO_DRAG_DROP yes drop
                .name_id       = 0,
                .item_class    = 25006,
                .item_type     = 20,
                .item_type_1   = 20,
                .category_id   = 11,
                .model_name    = STR8("Weapon_Empty.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 0,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 43001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [1] = {
                // Binoculars
                .defs_id       = 1542,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25054,
                .item_type     = 20,
                .item_type_1   = 20,
                .category_id   = 16,
                .model_name    = STR8("Weapon_Binoculars_3P.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [2] = {
                // Gas Runner Hoodie
                .defs_id       = 5747,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25002,
                .item_type     = 34,
                .item_type_1   = 34,
                .category_id   = 1,
                .model_name    = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 55001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [3] = {
                // Jeans
                .defs_id       = 2178,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25003,
                .item_type     = 34,
                .item_type_1   = 34,
                .category_id   = 3,
                .model_name    = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 55001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [4] = {
                // Conveys Sneakers
                .defs_id       = 2216,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25005,
                .item_type     = 28,
                .item_type_1   = 28,
                .category_id   = 107,
                .model_name    = STR8("SurvivorMale_Feet_Conveys.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 200,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 49001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
        },
    },
};

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_CommandItemDefinitions, &itemDefs);

    // 9. ReferenceData.WeaponDefinitions — fists weapon definition
    // Ref: H1emu zoneserver.ts sendInitData() → ReferenceData.WeaponDefinitions
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

    // 10. ClientUpdate.UpdateLocation — spawn position
    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = TRUE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

    // 11. ClientInitializationDetails — tells client init data is complete
    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientInitializationDetails, &initDetails);

    printf(MESSAGE_CONCAT_INFO("OnLogin complete — waiting for ClientIsReady\n"));
}