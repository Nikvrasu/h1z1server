void ZonePacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    if (dataLen == 0) {
        printf(MESSAGE_CONCAT_WARN("ZonePacketHandler called with 0 length data\n"));
        return;
    }
    printf("[ZONE] Incoming packet first bytes: 0x%02x 0x%02x 0x%02x (len=%u)\n", data[0],
           dataLen > 1 ? data[1] : 0, dataLen > 2 ? data[2] : 0, dataLen);
    Zone_Packet_Kind kind;
    printf("\n");

    __time64_t timer;
    _time64(&timer);

    u32 packetId = *data;

    u32 tempPacket;
    u32 packetIter;

    if (dataLen > 0) {
        for (packetIter = Zone_Packet_Kind_Unhandled + 1; packetIter < Zone_Packet_Kind__End;
             packetIter++) {
            if (data[0] == zone_registered_ids[packetIter]) {
                packetId = *data;
            }
        }
    }

    if (dataLen > 1) {
        tempPacket = (((0ul | data[0]) << 8) | data[1]);

        for (packetIter = Zone_Packet_Kind_Unhandled + 1; packetIter < Zone_Packet_Kind__End;
             packetIter++) {
            if (tempPacket == zone_registered_ids[packetIter]) {
                packetId = tempPacket;
                goto packetIdSwitch;
            }
        }
    }

    if (dataLen > 2) {
        tempPacket = ((0ul | data[0]) << 16) | endian_read_u16_little(data + 1);

        for (packetIter = Zone_Packet_Kind_Unhandled + 1; packetIter < Zone_Packet_Kind__End;
             packetIter++) {
            if (tempPacket == zone_registered_ids[packetIter]) {
                packetId = tempPacket;
                goto packetIdSwitch;
            }
        }
    }

packetIdSwitch:
    switch (packetId) {
        case ZONE_CLIENTISREADY_ID: {
            kind = Zone_Packet_Kind_ClientIsReady;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            if (session->isReady) {
                printf("[*] Ignoring duplicate ClientIsReady\n");
                break;
            }
            session->isReady = TRUE;

            DeployCharacter(app, session);
        } break;
        case ZONE_CLIENTFINISHEDLOADING_ID: {
            kind = Zone_Packet_Kind_ClientFinishedLoading;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            if (session->finished_loading) {
                printf("[*] Ignoring duplicate ClientFinishedLoading\n");
                break;
            }
            session->finished_loading = TRUE;

            // h1emu sends Equipment, WeaponStance, RunSpeed, ModifyMovementSpeed
            // in ClientFinishedLoading — NOT in DeployCharacter/ClientIsReady
            SendEquipmentAndMovement(app, session);

            printf("[*] ClientFinishedLoading acknowledged — equipment & movement sent\n");
        } break;
        case ZONE_GAMETIMESYNC_ID: {
            kind = Zone_Packet_Kind_GameTimeSync;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_GameTimeSync gameTimeSync = { 0 };

            gameTimeSync.cycle_speed = 10.f;
            gameTimeSync.time = 0x0bull;
            gameTimeSync.unk_bool = TRUE;

            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_GameTimeSync,
                           &gameTimeSync);
        } break;
        case ZONE_GETCONTINENTBATTLEINFO_ID: {
            kind = Zone_Packet_Kind_GetContinentBattleInfo;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_ContinentBattleInfo battleInfo = { 0 };

            battleInfo.zones_count = 1;
            battleInfo.zones = (struct zones_s[1]){
            [0] = {
                .continent_id = 1,
                .info_name_id = 1,
                .zone_description_id = 1,
                .zone_name = STR8("Z2"),
                .hex_size = 100,
                .is_production_zone = 1,
            },
        };

            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ContinentBattleInfo,
                           &battleInfo);
        } break;
        case ZONE_CLIENTINITIALIZATIONDETAILS_ID: {
            kind = Zone_Packet_Kind_ClientInitializationDetails;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;
        case ZONE_WALLOFDATA_UIEVENT_ID: {
            kind = Zone_Packet_Kind_WallOfData_UIEvent;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            // Zone_Packet_WallOfData_UIEvent uiEvent = { 0 };
            // zone_packet_unpack(data + 2, dataLen - 2, kind, &uiEvent, &app->arenaPerTick);
        } break;
        case ZONE_WALLOFDATA_CLIENTSYSTEMINFO_ID: {
            kind = Zone_Packet_Kind_WallOfData_ClientSystemInfo;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_WallOfData_ClientSystemInfo systemInfo = { 0 };
            zone_packet_unpack(data + 2, dataLen - 2, kind, &systemInfo, &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick, kind, &systemInfo);
        } break;
        case ZONE_WALLOFDATA_CLIENTTRANSITION_ID: {
            kind = Zone_Packet_Kind_WallOfData_ClientTransition;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_WallOfData_ClientTransition clientTransition = { 0 };
            zone_packet_unpack(data + 2, dataLen - 2, kind, &clientTransition, &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick, kind, &clientTransition);
        } break;
        case ZONE_SETLOCALE_ID: {
            kind = Zone_Packet_Kind_SetLocale;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_SetLocale setLocale = { 0 };
            setLocale.locale = STR8("en_US");

            ZonePacketSend(app, session, &app->arenaPerTick, kind, &setLocale);
        } break;
        case ZONE_CLIENTLOG_ID: {
            kind = Zone_Packet_Kind_ClientLog;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;
        case ZONE_CLIENTLOGOUT_ID: {
            kind = Zone_Packet_Kind_ClientLogout;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;
        case ZONE_LOBBYGAMEDEFINITION_DEFINITIONSREQUEST_ID: {
            kind = Zone_Packet_Kind_LobbyGameDefinition_DefinitionsRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;
        case ZONE_KEEPALIVE_ID: {
            kind = Zone_Packet_Kind_KeepAlive;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_KeepAlive keepAlive = { 0 };
            zone_packet_unpack(data + 1, dataLen - 1, kind, &keepAlive, &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick, kind, &keepAlive);
        } break;
        case ZONE_STATICVIEWREQUEST_ID: {
            kind = Zone_Packet_Kind_StaticViewRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            StaticViewBase(app, session, data, dataLen);
        } break;
        case ZONE_PLAYERWORLDTRANSFERREQUEST_ID: {
            kind = Zone_Packet_Kind_PlayerWorldTransferRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            // 1. Transfer confirm
            Zone_Packet_PlayerWorldTransferReply transferReply = { 0 };
            transferReply.world_id_reply = 1;
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_PlayerWorldTransferReply,
                           &transferReply);

            // 2. Initialization parameters
            Zone_Packet_InitializationParameters init_params = {
                .environment = STR8("LIVE_KOTK"),
            };
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                           &init_params);

            // 3. Zone Details
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

            // 4. Game Settings
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

            // 5. Location Update
            Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
                .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
                .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
                .trigger_loading_screen = TRUE,
                .unk_u8_1 = 0,
                .unk_bool = FALSE,
            };
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

            // 6. Reset loading flags for the new zone transition
            session->finished_loading = FALSE;
            session->isReady = FALSE;

                // ADD THESE before ClientBeginZoning:
            // ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096,  "..\\data\\ReferenceData_DynamicAppearance.bin");
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
            {
                u8 emptyItemDefs[] = {
                    0x09, 0x47, 0x00,  // opcode: CommandItemDefinitions (0x09 0x4700)
                    0x00, 0x00, 0x00, 0x00,  // stream length = 0 (empty stream)
                };
                // Actually we need stream length to include the list count:
                u8 emptyItemDefs2[] = {
                    0x09, 0x47, 0x00,           // opcode
                    0x04, 0x00, 0x00, 0x00,     // stream length = 4
                    0x00, 0x00, 0x00, 0x00,     // list count = 0 (no item defs)
                };
                u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyItemDefs2) + TunnelDataHeaderLen);
                memcpy(baseBuffer + TunnelDataHeaderLen, emptyItemDefs2, sizeof(emptyItemDefs2));
                GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyItemDefs2) + TunnelDataHeaderLen);
            }
            // ZonePacketRawFileSend(app, session, &app->arenaPerTick, 65536, "..\\data\\ReferenceData_WeaponDefinitions.bin");
            {
                u8 emptyWeaponDefs[] = {
                    0x17, 0x04,
                    0x10, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00,
                };
                u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyWeaponDefs) + TunnelDataHeaderLen);
                memcpy(baseBuffer + TunnelDataHeaderLen, emptyWeaponDefs, sizeof(emptyWeaponDefs));
                GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyWeaponDefs) + TunnelDataHeaderLen);
            }
            // ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096,  "..\\data\\ReferenceData_ProfileDefinitions.bin");
            // ZonePacketRawFileSend(app, session, &app->arenaPerTick, 8192,  "..\\data\\ReferenceData_ProjectileDefinitions.bin");
            // ZonePacketRawFileSend(app, session, &app->arenaPerTick, 4096,  "..\\data\\ReferenceData_ItemClassDefinitions.bin");

            // 7. Begin Zoning
            Zone_Packet_ClientBeginZoning beginZoning = { 0 };
            beginZoning.zone_name = STR8("Z2");
            beginZoning.zone_type = 4;
            beginZoning.pos = (vec4){ .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f };
            beginZoning.rot = (vec4){ .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f };
            beginZoning.overcast = 1.0f;
            beginZoning.fogDensity = 0.000173f;
            beginZoning.fogFloor = 10.0f;
            beginZoning.fogGradient = 0.0144f;
            beginZoning.globalPrecipitation = 0.0f;
            beginZoning.temperature = 75.0f;
            beginZoning.skyClarity = 0.0f;
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
            beginZoning.rainMinStrength = 0.0f;
            beginZoning.rainRampUpTimeSeconds = 1.0f;
            beginZoning.cloudFile = STR8("sky_Z_clouds.dds");
            beginZoning.stratusCloudTiling = 0.30f;
            beginZoning.stratusCloudScrollU = -0.002f;
            beginZoning.stratusCloudScrollV = 0.0f;
            beginZoning.stratusCloudHeight = 1000.0f;
            beginZoning.cumulusCloudTiling = 0.20f;
            beginZoning.cumulusCloudScrollU = 0.0f;
            beginZoning.cumulusCloudScrollV = 0.002f;
            beginZoning.cumulusCloudHeight = 8000.0f;
            beginZoning.cloudAnimationSpeed = 0.0f;
            beginZoning.cloudSilverLiningThickness = 0.25f;
            beginZoning.cloudSilverLiningBrightness = 7.0f;
            beginZoning.cloudShadows = 0.5f;
            beginZoning.unk_byte_1 = 4;
            beginZoning.zone_id_1 = 5;
            beginZoning.zone_id_2 = 5;
            beginZoning.name_id = 61609;
            beginZoning.unk_dword_1 = 0x0f2b07d0;
            beginZoning.unk_bool_1 = FALSE;
            beginZoning.wait_for_zone_ready = FALSE;
            beginZoning.unk_bool_2 = FALSE;
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientBeginZoning,
                           &beginZoning);

            // 8. Send character data
            SendSelfToClient(app, session, FALSE);

        } break;
        case 0x11: {
            // ClientUpdateBase — check sub-opcode
            u8 subOpcode = dataLen > 1 ? data[1] : 0;
            printf(MESSAGE_CONCAT_INFO("Handling ClientUpdateBase sub-opcode 0x%02x (len=%u)\n"),
                   subOpcode, dataLen);

            if (subOpcode == 0x97) {
                // 0x11 0x97 — Zone ready notification from client after a zone transition.
                printf(MESSAGE_CONCAT_INFO("Client reports zone ready! Deploying character...\n"));
                DeployCharacter(app, session);
            } else {
                printf(MESSAGE_CONCAT_WARN("Unhandled ClientUpdateBase sub-opcode 0x%02x\n"),
                       subOpcode);
            }
        } break;
        case 0x0f: {
            if (dataLen > 1 && data[1] == 0x45) {
                u64 requestedCharId = 0;
                if (dataLen >= 10) {
                    requestedCharId = endian_read_u64_little(data + 2);
                }
                printf("[*] FullCharacterDataRequest for 0x%llx — ignoring (self has full data from SendSelfToClient)\n",
                       (unsigned long long)requestedCharId);
                // Don't respond. The self-character already received full data via
                // SendSelfToClient. The old bare 0xdb byte was a corrupt packet that
                // may have been resetting the character's visual state.
            }
        } break;
        case 0x1144: {
            kind = Zone_Packet_Kind_ClientUpdate_MonitorTimeDrift;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ignored)\n"), zone_packet_names[kind]);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled Zone packet 0x%02x (len=%u)\n"), packetId, dataLen);
            // Hex dump first few bytes for debugging
            printf("[ZONE DUMP] ");
            for (u32 i = 0; i < (dataLen < 16 ? dataLen : 16); i++) {
                printf("%02x ", data[i]);
            }
            printf("\n");
        }
    }
}