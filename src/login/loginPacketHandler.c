static void LoginPersistCharacterName(AppState* app, SessionState* session, String8 name) {
    if (!name.size || !name.data) {
        return;
    }

    session->characterName.size = name.size;
    session->characterName.data = arena_push_size(&app->arenaTotal, name.size);
    memcpy(session->characterName.data, name.data, name.size);
}

static void LoginEnsureCharacterName(AppState* app, SessionState* session) {
    if (session->characterName.size == 0 || session->characterName.data == NULL) {
        LoginPersistCharacterName(app, session, STR8("Survivor"));
    }
}

static void LoginSendStructuredLoginReply(AppState* app, SessionState* session) {
    UNUSED(session);

    Login_Packet_LoginReply packetReply = { 0 };
    packetReply.is_logged_in = TRUE;
    packetReply.status = 1;
    packetReply.result_code = 1;
    packetReply.is_member = TRUE;
    packetReply.is_internal = TRUE;
    packetReply.namespace_name = STR8("soe");
    packetReply.account_features_count = 1;
    packetReply.account_features = (struct account_features_s[1]){
        [0] = {
            .key = 2,
            .id = 2,
            .active = TRUE,
            .remaining_count = 2,
            .raw_data = STR8("test"),
        },
    };
    packetReply.application_payload = STR8("");
    packetReply.error_details_count = 0;
    packetReply.error_details = NULL;
    packetReply.ip_country_code = STR8("");

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10), Login_Packet_Kind_LoginReply,
                    &packetReply);
}

static void LoginSendServerListReply(AppState* app, SessionState* session) {
    Login_Packet_ServerListReply packetReply = { 0 };

    packetReply.servers_count = 1;
    packetReply.servers = (struct servers_s[1]){
        {
            .id = 1,
            .state = 2,
            .is_locked = FALSE,
            .name = STR8("H1Z1-C-Server"),
            .name_id = 193,
            .description = STR8(
                "A server-emulator for H1Z1: King of the Kill, Preseason 3; Built in C"),
            .description_id = 1362,
            .server_info = STR8(
                "<ServerInfo Region=\"CharacterCreate.RegionUs\" Subregion=\"UI.SubregionUS\" "
                "IsRecommended=\"1\" IsRecommendedVS=\"0\" IsRecommendedNC=\"0\" "
                "IsRecommendedTR=\"0\" />"),
            .population_data =
                STR8("<Population PctCap=\"0\" PingAdr=\"127.0.0.1:60000\" Rulesets=\"\" "
                     "Mode=\"13\" "
                     "IsLogin=\"1\" IsWL=\"0\" IsEvt=\"0\" PL=\"0\" DC=\"LVS\" PopLock=\"0\" "
                     "GP=\"100\" BP=\"175\" MaxPop=\"4000\" Subregion=\"US\"><Fac "
                     "IsList=\"1\"/></Population>"),
            .is_access_allowed = TRUE,
        },
    };

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10), Login_Packet_Kind_ServerListReply,
                    &packetReply);
}

void LoginPacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    if (dataLen == 0) {
        printf(MESSAGE_CONCAT_WARN("LoginPacketHandler called with empty packet\n"));
        return;
    }

    printf("\n");

    Login_Packet_Kind kind;
    u8 packetId = data[0];
    i32 offset = sizeof(u8);

    switch (packetId) {
        case LOGIN_LOGINREQUEST_ID: {
            kind = Login_Packet_Kind_LoginRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            LoginSendStructuredLoginReply(app, session);
            session->first_login = TRUE;
            session->loginPhase = LoginFlowPhase_LoginRequestHandled;
        } break;

        case LOGIN_SERVERLISTREQUEST_ID: {
            kind = Login_Packet_Kind_ServerListRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            if (session->loginPhase < LoginFlowPhase_LoginRequestHandled) {
                printf(MESSAGE_CONCAT_WARN("ServerListRequest received before LoginRequest; serving anyway\n"));
            }

            LoginSendServerListReply(app, session);
            if (session->loginPhase < LoginFlowPhase_ServerListSent) {
                session->loginPhase = LoginFlowPhase_ServerListSent;
            }
        } break;

        case LOGIN_TUNNELAPPPACKETCLIENTTOSERVER_ID: {
            kind = Login_Packet_Kind_TunnelAppPacketClientToServer;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            NameValidation(app, session, data, dataLen);

            if (session->loginPhase < LoginFlowPhase_CharacterFlowReady) {
                session->loginPhase = LoginFlowPhase_CharacterFlowReady;
            }
        } break;

        case LOGIN_CHARACTERCREATEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterCreateRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterCreateRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            session->selected_server_id = packet.server_id;
            if (packet.char_payload) {
                GetHeadTypeId(session, &packet);
                LoginPersistCharacterName(app, session, packet.char_payload->character_name);
            }

            CharacterCreate(app, session);

            if (session->loginPhase < LoginFlowPhase_CharacterFlowReady) {
                session->loginPhase = LoginFlowPhase_CharacterFlowReady;
            }
        } break;

        case LOGIN_CHARACTERSELECTINFOREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterSelectInfoRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            if (session->characterId == 0) {
                session->characterId = generateRandomGuid();
                session->createReply.status = 1;
            }
            LoginEnsureCharacterName(app, session);

            CharacterSelectInfo(app, session);

            if (session->loginPhase < LoginFlowPhase_CharacterFlowReady) {
                session->loginPhase = LoginFlowPhase_CharacterFlowReady;
            }
        } break;

        case LOGIN_CHARACTERLOGINREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterLoginRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterLoginRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            LoginEnsureCharacterName(app, session);

            if (packet.server_id) {
                session->selected_server_id = packet.server_id;
            }
            if (packet.character_id) {
                session->characterId = packet.character_id;
            }
            if (session->selected_server_id == 0) {
                session->selected_server_id = 1;
            }
            if (session->characterId == 0) {
                session->characterId = generateRandomGuid();
            }

            String8 characterName = session->characterName;

            Login_Packet_CharacterLoginReply packetReply = { 0 };
            packetReply.character_id = session->characterId;
            packetReply.server_id = session->selected_server_id;
            packetReply.last_login = 0;
            packetReply.status = 1;
            packetReply.login_payload_length = 1;

            char ticketBuf[256];
            u64 steamLikeId = 76561197960265728ull + (session->characterId & 0xffffffffull);
            snprintf(ticketBuf, sizeof(ticketBuf), "%llu:%.*s",
                     (unsigned long long)steamLikeId,
                     (int)characterName.size,
                     characterName.data);

            packetReply.login_payload = (struct login_payload_s[1]){
                {
                    .unk_byte_1 = 1,
                    .unk_byte_2 = 0,
                    .server_address = STR8("127.0.0.1:60000"),
                    .server_ticket = string8_make((u8*)ticketBuf, strlen(ticketBuf)),
                    .encryption_key =
                        STR8("\x17\xbd\x08\x6b\x1b\x94\xf0\x2f\xf0\xec\x53\xd7\x63\x58\x9b\x5f"),
                    .soe_protocol_version = 3,
                    .character_id = session->characterId,
                    .unk_u64 = 0,
                    .station_name = STR8(""),
                    .character_name = characterName,
                    .unk_str = STR8(""),
                    .server_feature_bit = 0,
                },
            };

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterLoginReply, &packetReply);

            session->loginPhase = LoginFlowPhase_CharacterLoginAccepted;
        } break;

        case LOGIN_CHARACTERDELETEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterDeleteRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterDeleteRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            Login_Packet_CharacterDeleteReply packetReply = { 0 };
            packetReply.character_id = packet.character_id;
            packetReply.status = 1;
            packetReply.payload3 = STR8("");

            if (session->characterId == packet.character_id) {
                session->characterId = 0;
            }

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterDeleteReply, &packetReply);
        } break;

        case LOGIN_LOGOUT_ID: {
            kind = Login_Packet_Kind_Logout;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
            session->loginPhase = LoginFlowPhase_Connected;
        } break;

        case LOGIN_FORCEDISCONNECT_ID: {
            kind = Login_Packet_Kind_ForceDisconnect;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
        } break;

        case 0x6b:
        case 0x79:
        case 0xb7: {
            printf(MESSAGE_CONCAT_INFO("Received unknown packet 0x%02x (ignoring)\n"), packetId);
        } break;

        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled login packet 0x%02x\n"), packetId);
        } break;
    }
}