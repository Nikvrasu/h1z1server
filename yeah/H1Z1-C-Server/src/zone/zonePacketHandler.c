void ZonePacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    if (dataLen == 0) {
        printf(MESSAGE_CONCAT_WARN("ZonePacketHandler called with 0 length data\n"));
        return;
    }
    printf("[ZONE] Incoming packet first bytes: 0x%02x 0x%02x 0x%02x (len=%u)\n",
           data[0], dataLen > 1 ? data[1] : 0, dataLen > 2 ? data[2] : 0, dataLen);
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
            // session->isReady = TRUE;

            // Phase 2: client has finished loading the zone — deploy the character.
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

            setEquipment.equipment_slot_array_count = 1;
            setEquipment.equipment_slot_array = (struct equipment_slot_array_s[1]){
            [0] = {
                .length_2 = (struct length_2_s[1]){
                    [0] = {
                        .tint_alias = STR8("Default"),
                        .decal_alias = STR8("#"),
                    },
                },
            },
        };

            setEquipment.attachments_data_1_count = 1;

            setEquipment.attachments_data_1 = (struct attachments_data_1_s[1]){
            [0] = {
                .tint_alias = STR8("Default"),
                .decal_alias = STR8("#"),
            },
        };

            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);

            Zone_Packet_Loadout_SetLoadoutSlots setLoadoutSlots = { 0 };

            setLoadoutSlots.character_id = session->characterId;
            setLoadoutSlots.loadout_slot_data_count = 1;

            setLoadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[1]){
            [0] = {
                .unk_byte_1 = 255,
            },
        };

            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_Loadout_SetLoadoutSlots,
                           &setLoadoutSlots);
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

            Zone_Packet_WallOfData_UIEvent uiEvent = { 0 };
            zone_packet_unpack(data + 2, dataLen - 2, kind, &uiEvent, &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick, kind, &uiEvent);
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

            // Zone_Packet_ClientLog logPacket = { 0 };
            // zone_packet_unpack(data + 1, dataLen - 1, kind, &logPacket, &app->arenaPerTick);
            // printf("[CLIENT LOG] file: %.*s message: %.*s\n",
            //     (int)logPacket.file.size, logPacket.file.data,
            //     (int)logPacket.message.size, logPacket.message.data);
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

            // 1. Transfer bestätigen
            Zone_Packet_PlayerWorldTransferReply tranferReply = { 0 };
            tranferReply.world_id_reply = 1;
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_PlayerWorldTransferReply, 
                        &tranferReply);

            // 2. Initialisierungsparameter (Wie von dir ergänzt)
            Zone_Packet_InitializationParameters init_params = { .environment = STR8("LIVE_KOTK") };
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters, 
                        &init_params);

            // 3. Zone Details & Settings (Platzhalter für Login-Daten)
            // Hier die gleichen Daten wie in der OnLogin-Prozedur verwenden
            Zone_Packet_SendZoneDetails send_zone_details = { /* Gleiche Daten wie OnLogin */ };
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendZoneDetails, 
                        &send_zone_details);

            Zone_Packet_ClientGameSettings game_settings = { /* Gleiche Daten wie OnLogin */ };
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientGameSettings, 
                        &game_settings);

            // 4. Location Update (Teilt dem Client die neuen Koordinaten mit)
            Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
                .position = { .x = 1000.f, .y = 1000.f, .z = 1000.f, .w = 1.f },
                .rotation = { .x = 0.f, .y = 0.f, .z = 0.f, .w = 1.f },
                .trigger_loading_screen = TRUE,
                .unk_u8_1 = 0,
                .unk_bool = FALSE,
            };
            ZonePacketSend(app, session, &app->arenaPerTick, 
                        Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

            // 5. Zoning Prozess starten
            Zone_Packet_ClientBeginZoning beginZoning = {
                .zone_name = STR8("Z2"),
                .zone_type = 4,
                .pos = { .x = 1000.f, .y = 1000.f, .z = 1000.f, .w = 1.f },
                .rot = { .x = 0.f, .y = 0.f, .z = 0.f, .w = 1.f },
                .cloudFile = STR8(""),
                .unk_byte_1 = 5,
                .zone_id_1 = 5,
                .zone_id_2 = 0,
                .name_id = 7699,
                .unk_dword_1 = 674234378,
                .unk_bool_1 = FALSE,
                .wait_for_zone_ready = FALSE,
                .unk_bool_2 = FALSE,
            };
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientBeginZoning, 
                        &beginZoning);

            // 6. Eigene Charakter-Daten senden
            SendSelfToClient(app, session);

        } break;
        case 0x11: {
            // ClientUpdateBase — check sub-opcode
            u8 subOpcode = dataLen > 1 ? data[1] : 0;
            printf(MESSAGE_CONCAT_INFO("Handling ClientUpdateBase sub-opcode 0x%02x (len=%u)\n"),
                   subOpcode, dataLen);

            if (subOpcode == 0x97) {
                // 0x11 0x97 — Zone ready notification from client after a zone transition.
                // Re-deploy the character following the same Phase 2 sequence.
                printf(MESSAGE_CONCAT_INFO("Client reports zone ready! Deploying character...\n"));

                DeployCharacter(app, session);
            } else {
                printf(MESSAGE_CONCAT_WARN("Unhandled ClientUpdateBase sub-opcode 0x%02x\n"), subOpcode);
            }
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