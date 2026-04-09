// ============================================================================
// Zone Packet Handler
//
// Routes incoming zone/client protocol packets to their handlers.
// Ref: H1emu/h1z1-server src/servers/ZoneServer2016/zoneserver.ts
//
// Packet ID resolution: variable-length opcodes (1-3 bytes) matched against
// the registered zone_registered_ids[] table.
//
// Key flow:
//   ClientIsReady (0x04)          → DeployCharacter (phase 2)
//   ClientFinishedLoading (0x06)  → mark finished_loading
//   PlayerWorldTransferRequest    → ClientBeginZoning + reset lifecycle
//   ClientUpdateBase 0x97         → zone ready → DeployCharacter
// ============================================================================
void ZonePacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    if (dataLen == 0) {
        printf(MESSAGE_CONCAT_WARN("ZonePacketHandler: empty packet\n"));
        return;
    }

    Zone_Packet_Kind kind;
    __time64_t timer;
    _time64(&timer);

    // Resolve packet ID (1-3 byte variable-length opcodes)
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
        // ================================================================
        // ClientIsReady — client has loaded the zone, deploy character
        // Ref: H1emu zoneserver.ts onClientIsReady()
        // ================================================================
        case ZONE_CLIENTISREADY_ID: {
            kind = Zone_Packet_Kind_ClientIsReady;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), zone_packet_names[kind]);

            if (session->isReady) {
                printf(MESSAGE_CONCAT_WARN("Duplicate ClientIsReady — ignoring\n"));
                break;
            }
            session->isReady = TRUE;
            DeployCharacter(app, session);
        } break;
        // ================================================================
        // ClientFinishedLoading — client ready for gameplay
        // Ref: H1emu zoneserver.ts onClientFinishedLoading()
        // ================================================================
        case ZONE_CLIENTFINISHEDLOADING_ID: {
            kind = Zone_Packet_Kind_ClientFinishedLoading;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), zone_packet_names[kind]);

            if (session->finished_loading) {
                printf(MESSAGE_CONCAT_WARN("Duplicate ClientFinishedLoading — ignoring\n"));
                break;
            }
            session->finished_loading = TRUE;
        } break;
        // ================================================================
        // GameTimeSync — client requests time synchronization
        // ================================================================
        case ZONE_GAMETIMESYNC_ID: {
            kind = Zone_Packet_Kind_GameTimeSync;

            Zone_Packet_GameTimeSync gameTimeSync = { 0 };
            gameTimeSync.cycle_speed = 0.0f;
            gameTimeSync.time = 300000;
            gameTimeSync.unk_bool = TRUE;
            ZonePacketSend(app, session, &app->arenaPerTick, kind, &gameTimeSync);
        } break;
        // ================================================================
        // GetContinentBattleInfo — client requests map/continent info
        // ================================================================
        case ZONE_GETCONTINENTBATTLEINFO_ID: {
            kind = Zone_Packet_Kind_GetContinentBattleInfo;

            Zone_Packet_ContinentBattleInfo battleInfo = { 0 };
            battleInfo.zones_count = 1;
            battleInfo.zones = (struct zones_s[1]){{
                .continent_id = 1,
                .info_name_id = 1,
                .zone_description_id = 1,
                .zone_name = STR8("Z2"),
                .hex_size = 100,
                .is_production_zone = 1,
            }};
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_ContinentBattleInfo, &battleInfo);
        } break;
        // ================================================================
        // ClientInitializationDetails — acknowledgment (no-op)
        // ================================================================
        case ZONE_CLIENTINITIALIZATIONDETAILS_ID: {
        } break;

        // ================================================================
        // WallOfData — telemetry packets (UI events, system info, transitions)
        // DO NOT echo back
        // ================================================================
        case ZONE_WALLOFDATA_UIEVENT_ID: {
        } break;

        case ZONE_WALLOFDATA_CLIENTSYSTEMINFO_ID: {
        } break;

        case ZONE_WALLOFDATA_CLIENTTRANSITION_ID: {
        } break;

        // ================================================================
        // SetLocale — client locale preference
        // ================================================================
        case ZONE_SETLOCALE_ID: {
            Zone_Packet_SetLocale setLocale = { 0 };
            setLocale.locale = STR8("en_US");
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SetLocale, &setLocale);
        } break;

        // ================================================================
        // ClientLog — client-side log message (informational)
        // ================================================================
        case ZONE_CLIENTLOG_ID: {
        } break;

        // ================================================================
        // ClientLogout — client requests to return to main menu
        // Ref: H1emu zoneserver.ts ClientLogout handler
        // ================================================================
        case ZONE_CLIENTLOGOUT_ID: {
            printf(MESSAGE_CONCAT_INFO("Received ClientLogout\n"));
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_ClientUpdate_CompleteLogoutProcess, 0);
        } break;

        // ================================================================
        // LobbyGameDefinition.DefinitionsRequest — lobby definition query
        // ================================================================
        case ZONE_LOBBYGAMEDEFINITION_DEFINITIONSREQUEST_ID: {
        } break;

        // ================================================================
        // KeepAlive — connection heartbeat, echo back
        // ================================================================
        case ZONE_KEEPALIVE_ID: {
            Zone_Packet_KeepAlive keepAlive = { 0 };
            zone_packet_unpack(data + 1, dataLen - 1, Zone_Packet_Kind_KeepAlive,
                               &keepAlive, &app->arenaPerTick);
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_KeepAlive, &keepAlive);
        } break;
        // ================================================================
        // StaticViewRequest — client requests static map view data
        // ================================================================
        case ZONE_STATICVIEWREQUEST_ID: {
            StaticViewBase(app, session, data, dataLen);
        } break;

        // ================================================================
        // PlayerWorldTransferRequest — matchmaking zone transfer
        // Sends ClientBeginZoning and resets lifecycle flags
        // Ref: H1emu zoneserver.ts PlayerWorldTransferRequest handler
        // ================================================================
        case ZONE_PLAYERWORLDTRANSFERREQUEST_ID: {
            printf(MESSAGE_CONCAT_INFO("Received PlayerWorldTransferRequest\n"));

            // Guard: block duplicate transfers during transition
            if (!session->isReady && !session->finished_loading && !session->characterDeployed) {

                Zone_Packet_PlayerWorldTransferReply transferReply = { 0 };
                transferReply.world_id_reply = 1;
                ZonePacketSend(app, session, &app->arenaPerTick,
                            Zone_Packet_Kind_PlayerWorldTransferReply, &transferReply);
                break;
            }

            // 1. Transfer reply
            Zone_Packet_PlayerWorldTransferReply transferReply = { 0 };
            transferReply.world_id_reply = 1;
            ZonePacketSend(app, session, &app->arenaPerTick,
                        Zone_Packet_Kind_PlayerWorldTransferReply, &transferReply);

            // 2. ClientBeginZoning — triggers the zone load
            Zone_Packet_ClientBeginZoning beginZoning = { 0 };
            beginZoning.zone_name                  = STR8("Z2");
            beginZoning.zone_type                  = 4;
            beginZoning.pos                        = (vec4){ .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f };
            beginZoning.rot                        = (vec4){ .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f };
            beginZoning.overcast                   = 1.0f;
            beginZoning.fogDensity                 = 0.000173f;
            beginZoning.fogFloor                   = 10.0f;
            beginZoning.fogGradient                = 0.0144f;
            beginZoning.globalPrecipitation        = 0.0f;
            beginZoning.temperature                = 75.0f;
            beginZoning.skyClarity                 = 0.0f;
            beginZoning.cloudWeight0               = 0.05f;
            beginZoning.cloudWeight1               = 0.0f;
            beginZoning.cloudWeight2               = 0.05f;
            beginZoning.cloudWeight3               = 0.15f;
            beginZoning.transitionTime             = 0.0f;
            beginZoning.sunAxisX                   = 38.0f;
            beginZoning.sunAxisY                   = -15.0f;
            beginZoning.sunAxisZ                   = 0.0f;
            beginZoning.windDirX                   = -1.0f;
            beginZoning.windDirY                   = -0.5f;
            beginZoning.windDirZ                   = -1.0f;
            beginZoning.wind                       = 3.0f;
            beginZoning.rainMinStrength            = 0.0f;
            beginZoning.rainRampUpTimeSeconds      = 1.0f;
            beginZoning.cloudFile                  = STR8("sky_Z_clouds.dds");
            beginZoning.stratusCloudTiling         = 0.30f;
            beginZoning.stratusCloudScrollU        = -0.002f;
            beginZoning.stratusCloudScrollV        = 0.0f;
            beginZoning.stratusCloudHeight         = 1000.0f;
            beginZoning.cumulusCloudTiling         = 0.20f;
            beginZoning.cumulusCloudScrollU        = 0.0f;
            beginZoning.cumulusCloudScrollV        = 0.002f;
            beginZoning.cumulusCloudHeight         = 8000.0f;
            beginZoning.cloudAnimationSpeed        = 0.0f;
            beginZoning.cloudSilverLiningThickness = 0.25f;
            beginZoning.cloudSilverLiningBrightness = 7.0f;
            beginZoning.cloudShadows               = 0.5f;
            beginZoning.unk_byte_1                 = 4;
            beginZoning.zone_id_1                  = 5;
            beginZoning.zone_id_2                  = 5;
            beginZoning.name_id                    = 61609;
            beginZoning.unk_dword_1                = 0x0f2b07d0;
            beginZoning.unk_bool_1                 = FALSE;
            beginZoning.wait_for_zone_ready        = FALSE;
            beginZoning.unk_bool_2                 = FALSE;
            ZonePacketSend(app, session, &app->arenaPerTick,
                        Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

            // 3. Reset lifecycle flags
            session->finished_loading  = FALSE;
            session->isReady           = FALSE;
            session->characterReleased = FALSE;
            session->characterDeployed = FALSE;
            session->zoneCycleId += 1;
            if (session->zoneCycleId == 0) session->zoneCycleId = 1;

            printf(MESSAGE_CONCAT_INFO("Transfer: reset lifecycle, cycle=%u\n"),
                   session->zoneCycleId);
        } break;

        // ================================================================
        // ClientUpdateBase (0x11) — variable sub-opcodes
        //   0x97 = zone ready notification → DeployCharacter
        // ================================================================
        case 0x11: {
            u8 subOpcode = dataLen > 1 ? data[1] : 0;
            if (subOpcode == 0x97) {
                printf(MESSAGE_CONCAT_INFO("Client zone ready (0x11 0x97) — deploying\n"));
                DeployCharacter(app, session);
            }
        } break;

        // ================================================================
        // FullCharacterDataRequest (0x0f 0x45) — ignored for KotK
        // ================================================================
        case 0x0f: {
        } break;

        // ================================================================
        // MonitorTimeDrift (0x1144) — telemetry, ignored
        // ================================================================
        case 0x1144: {
        } break;

        // ================================================================
        // Command.InteractRequest — interaction with world objects
        // ================================================================
        case ZONE_COMMAND_INTERACTREQUEST_ID: {
            ZonePacketSend(app, session, &app->arenaPerTick,
                        Zone_Packet_Kind_Command_InteractCancel, 0);
        } break;

        case ZONE_COMMAND_INTERACTCANCEL_ID: {
        } break;

        case ZONE_COMMAND_INTERACTIONLIST_ID: {
        } break;

        // ================================================================
        // Synchronization (0x8d) — time sync with server timestamps
        // ================================================================
        case 0x8d: {
            __time64_t now;
            _time64(&now);
            u64 serverTimeMs = (u64)now * 1000;

            Zone_Packet_Synchronization sync = { 0 };
            sync.client_hours_ms  = endian_read_u64_little(data + 1);
            sync.client_hours_ms2 = endian_read_u64_little(data + 9);
            sync.client_time      = endian_read_u64_little(data + 17);
            sync.server_time      = serverTimeMs;
            sync.server_time_2    = serverTimeMs;
            sync.unk_time         = 0;
            ZonePacketSend(app, session, &app->arenaPerTick,
                        Zone_Packet_Kind_Synchronization, &sync);
        } break;

        // ================================================================
        // Command.SetProfile — profile change request (ignored)
        // ================================================================
        case ZONE_COMMAND_SETPROFILE_ID: {
        } break;

        // ================================================================
        // CharacterSelectSessionRequest (0xc4) — session response
        // ================================================================
        case 0xc4: {
            u8 sessionResponse[] = { 0xc5 };
            u8* buf = arena_push_size(&app->arenaPerTick, sizeof(sessionResponse) + TunnelDataHeaderLen);
            memcpy(buf + TunnelDataHeaderLen, sessionResponse, sizeof(sessionResponse));
            GatewayTunnelDataSend(app, session, buf, sizeof(sessionResponse) + TunnelDataHeaderLen);
        } break;

        // ================================================================
        // Unknown packets
        // ================================================================
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled zone packet 0x%02x (len=%u)\n"), packetId, dataLen);
        }
    }
}