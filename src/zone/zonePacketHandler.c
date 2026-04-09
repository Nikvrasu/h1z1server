// ============================================================================
// ZonePacketHandler — Main dispatch for incoming zone protocol packets
//
// Based on H1emu/h1z1-server ZoneServer2016 packet routing.
//
// Packet categories:
//   Connection lifecycle: ClientIsReady, ClientFinishedLoading, ClientLogout
//   Time/sync: GameTimeSync, Synchronization, KeepAlive
//   World: GetContinentBattleInfo, StaticViewRequest, PlayerWorldTransfer
//   Client state: WallOfData, SetLocale, ClientLog, MonitorTimeDrift
//   Interaction: Command.InteractRequest, InteractCancel, InteractionList
//   Lobby: LobbyGameDefinition.DefinitionsRequest
//   UI: CharacterSelectSessionRequest
//   Player update: ClientUpdateBase, FullCharacterDataRequest
// ============================================================================
void ZonePacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    if (dataLen == 0) {
        printf(MESSAGE_CONCAT_WARN("ZonePacketHandler called with 0 length data\n"));
        return;
    }

    Zone_Packet_Kind kind;
    __time64_t timer;
    _time64(&timer);

    // ========================================================================
    // Packet ID resolution — supports 1, 2, or 3-byte opcodes
    // Scans registered packet IDs to find the correct match
    // ========================================================================
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

        // ====================================================================
        // ClientIsReady — Client signals it's ready for character deployment
        // H1emu: onClientIsReady() → sendCharacterData, deployCharacter
        // ====================================================================
        case ZONE_CLIENTISREADY_ID: {
            kind = Zone_Packet_Kind_ClientIsReady;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            if (session->isReady) {
                printf("[ZONE] Ignoring duplicate ClientIsReady\n");
                break;
            }
            session->isReady = TRUE;
            DeployCharacter(app, session);
        } break;

        // ====================================================================
        // ClientFinishedLoading — Client reports zone assets loaded
        // H1emu: onClientFinishedLoading() — acknowledge only
        // ====================================================================
        case ZONE_CLIENTFINISHEDLOADING_ID: {
            kind = Zone_Packet_Kind_ClientFinishedLoading;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            if (session->finished_loading) {
                printf("[ZONE] Ignoring duplicate ClientFinishedLoading\n");
                break;
            }
            session->finished_loading = TRUE;
        } break;

        // ====================================================================
        // GameTimeSync — Client requests time synchronization
        // ====================================================================
        case ZONE_GAMETIMESYNC_ID: {
            kind = Zone_Packet_Kind_GameTimeSync;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_GameTimeSync gameTimeSync = { 0 };
            gameTimeSync.cycle_speed = 0.0f;
            gameTimeSync.time = 300000;
            gameTimeSync.unk_bool = TRUE;
            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_GameTimeSync,
                           &gameTimeSync);
        } break;

        // ====================================================================
        // GetContinentBattleInfo — Client requests map/zone information
        // H1emu: sendContinentBattleInfo()
        // ====================================================================
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

        // ====================================================================
        // ClientInitializationDetails — acknowledged, no response needed
        // ====================================================================
        case ZONE_CLIENTINITIALIZATIONDETAILS_ID: {
            kind = Zone_Packet_Kind_ClientInitializationDetails;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;

        // ====================================================================
        // WallOfData — Client telemetry (UIEvent, SystemInfo, Transition)
        // H1emu: these are telemetry-only, never echo back
        // ====================================================================
        case ZONE_WALLOFDATA_UIEVENT_ID: {
            // UI event spam — silently ignore
        } break;
        case ZONE_WALLOFDATA_CLIENTSYSTEMINFO_ID: {
            printf(MESSAGE_CONCAT_INFO("WallOfData.ClientSystemInfo (telemetry, ignored)\n"));
        } break;
        case ZONE_WALLOFDATA_CLIENTTRANSITION_ID: {
            printf(MESSAGE_CONCAT_INFO("WallOfData.ClientTransition (state notification, ignored)\n"));
        } break;

        // ====================================================================
        // SetLocale — Client language setting
        // ====================================================================
        case ZONE_SETLOCALE_ID: {
            kind = Zone_Packet_Kind_SetLocale;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            Zone_Packet_SetLocale setLocale = { 0 };
            setLocale.locale = STR8("en_US");
            ZonePacketSend(app, session, &app->arenaPerTick, kind, &setLocale);
        } break;

        // ====================================================================
        // ClientLog — Client debug log, acknowledged only
        // ====================================================================
        case ZONE_CLIENTLOG_ID: {
            kind = Zone_Packet_Kind_ClientLog;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;

        // ====================================================================
        // ClientLogout — Client wants to return to main menu
        // H1emu: onClientLogout() → CompleteLogoutProcess
        // ====================================================================
        case ZONE_CLIENTLOGOUT_ID: {
            kind = Zone_Packet_Kind_ClientLogout;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_ClientUpdate_CompleteLogoutProcess, 0);
            printf("[LOGOUT] Sent CompleteLogoutProcess\n");
        } break;

        // ====================================================================
        // LobbyGameDefinition.DefinitionsRequest
        // ====================================================================
        case ZONE_LOBBYGAMEDEFINITION_DEFINITIONSREQUEST_ID: {
            kind = Zone_Packet_Kind_LobbyGameDefinition_DefinitionsRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
        } break;

        // ====================================================================
        // KeepAlive — Echo back to maintain connection
        // ====================================================================
        case ZONE_KEEPALIVE_ID: {
            kind = Zone_Packet_Kind_KeepAlive;

            Zone_Packet_KeepAlive keepAlive = { 0 };
            zone_packet_unpack(data + 1, dataLen - 1, kind, &keepAlive, &app->arenaPerTick);
            ZonePacketSend(app, session, &app->arenaPerTick, kind, &keepAlive);
        } break;

        // ====================================================================
        // StaticViewRequest — Client requests a camera viewpoint
        // ====================================================================
        case ZONE_STATICVIEWREQUEST_ID: {
            kind = Zone_Packet_Kind_StaticViewRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);
            StaticViewBase(app, session, data, dataLen);
        } break;

        // ====================================================================
        // PlayerWorldTransferRequest — Matchmaking zone transition
        // H1emu: onPlayerWorldTransferRequest()
        // ====================================================================
        case ZONE_PLAYERWORLDTRANSFERREQUEST_ID: {
            kind = Zone_Packet_Kind_PlayerWorldTransferRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s\n"), zone_packet_names[kind]);

            // Guard: if transfer is already in progress, acknowledge only
            if (!session->isReady && !session->finished_loading && !session->characterDeployed) {
                printf("[TRANSFER] Duplicate transfer while in progress; reply-only\n");
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

            // 3. Reset lifecycle flags for new zone cycle
            session->finished_loading  = FALSE;
            session->isReady           = FALSE;
            session->characterReleased = FALSE;
            session->characterDeployed = FALSE;
            session->zoneCycleId += 1;
            if (session->zoneCycleId == 0) session->zoneCycleId = 1;

            printf("[TRANSFER] Zone cycle reset: zoneCycleId=%u\n", session->zoneCycleId);
        } break;

        // ====================================================================
        // ClientUpdateBase (0x11) — Sub-opcode dispatch
        // 0x11 0x97 = Zone ready after transition → deploy character
        // ====================================================================
        case 0x11: {
            u8 subOpcode = dataLen > 1 ? data[1] : 0;
            if (subOpcode == 0x97) {
                printf(MESSAGE_CONCAT_INFO("ClientUpdateBase: zone ready → DeployCharacter\n"));
                DeployCharacter(app, session);
            }
        } break;

        // ====================================================================
        // FullCharacterDataRequest (0x0f 0x45) — ignored in KotK
        // ====================================================================
        case 0x0f: {
            if (dataLen > 1 && data[1] == 0x45) {
                // KotK client never uses this; silently ignore
            }
        } break;

        // ====================================================================
        // MonitorTimeDrift (0x1144) — telemetry, ignored
        // ====================================================================
        case 0x1144: {
            // Client time drift monitoring — no response needed
        } break;

        // ====================================================================
        // Interaction commands
        // ====================================================================
        case ZONE_COMMAND_INTERACTREQUEST_ID: {
            printf(MESSAGE_CONCAT_INFO("Handling Command.InteractRequest\n"));
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_Command_InteractCancel, 0);
        } break;
        case ZONE_COMMAND_INTERACTCANCEL_ID: {
            // Acknowledged
        } break;
        case ZONE_COMMAND_INTERACTIONLIST_ID: {
            // Acknowledged
        } break;

        // ====================================================================
        // Synchronization (0x8d) — Time sync echo
        // ====================================================================
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

        // ====================================================================
        // Command.SetProfile — acknowledged, no response
        // ====================================================================
        case ZONE_COMMAND_SETPROFILE_ID: {
            // Profile change request — acknowledged
        } break;

        // ====================================================================
        // CharacterSelectSessionRequest (0xc4) → reply 0xc5
        // ====================================================================
        case 0xc4: {
            printf(MESSAGE_CONCAT_INFO("Handling CharacterSelectSessionRequest\n"));
            u8 sessionResponse[] = { 0xc5 };
            u8* buf = arena_push_size(&app->arenaPerTick, sizeof(sessionResponse) + TunnelDataHeaderLen);
            memcpy(buf + TunnelDataHeaderLen, sessionResponse, sizeof(sessionResponse));
            GatewayTunnelDataSend(app, session, buf, sizeof(sessionResponse) + TunnelDataHeaderLen);
        } break;

        // ====================================================================
        // Unknown/unhandled packets
        // ====================================================================
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled zone packet 0x%02x (len=%u)\n"), packetId, dataLen);
        }
    }
}