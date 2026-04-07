void LoginPacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind;
    printf("\n");

    u8 packetId = *data;
    i32 offset = sizeof(u8);

    switch (packetId) {
        case LOGIN_LOGINREQUEST_ID: {
            kind = Login_Packet_Kind_LoginRequest;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);

            /*
            Login_Packet_LoginRequest packet = {0};
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            Login_Packet_LoginReply packetReply = {0};

            packetReply.is_logged_in = 1;
            packetReply.status = 1;
            packetReply.result_code = 1;
            packetReply.is_member = 1;
            packetReply.is_internal = 1;

            packetReply.account_features_count = 1;
            packetReply.account_features = (struct account_features_s[1]){
                [0] = {
                    0,
                },
            };

            packetReply.error_details_count = 1;
            packetReply.error_details = (struct error_details_s[1]){
                [0] = {
                    0,
                },
            };

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10), Login_Packet_Kind_LoginReply,
            &packetReply);
            */

            // Send the edited raw data, captured from Z1BR to work with KotK and JS
            LoginPacketRawFileSend(app, session, &app->arenaPerTick, KB(20),
                                   "..\\src\\login\\loginReply.bin");
        } break;
        case LOGIN_LOGOUT_ID: {
            kind = Login_Packet_Kind_Logout;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);
        } break;
        case LOGIN_FORCEDISCONNECT_ID: {
            kind = Login_Packet_Kind_ForceDisconnect;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);
        } break;
        case LOGIN_SERVERLISTREQUEST_ID: {
            kind = Login_Packet_Kind_ServerListRequest;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);

            Login_Packet_ServerListReply packetReply = { 0 };

            packetReply.servers_count = 1;

            packetReply.servers = (struct servers_s[1]){
                {
                    .id = 1,
                    .state = 2,
                    .is_locked = FALSE,
                    .name = STR8("H1Z1-C-Server"),
                    .name_id = 193,
                    .description =
                        STR8("A server-emulator for H1Z1: King of the Kill, Preseason 3; Built in C"),
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
        } break;
        case LOGIN_CHARACTERDELETEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterDeleteRequest;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterDeleteRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            Login_Packet_CharacterDeleteReply packetReply = { 0 };

            packetReply.character_id = session->characterId;
            packetReply.status = 1;

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterDeleteReply, &packetReply);
        } break;
        case LOGIN_TUNNELAPPPACKETCLIENTTOSERVER_ID: {
            NameValidation(app, session, data, dataLen);
        } break;
        case LOGIN_CHARACTERCREATEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterCreateRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterCreateRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            GetHeadTypeId(session, &packet);
            CharacterCreate(app, session);
        } break;
        case LOGIN_CHARACTERSELECTINFOREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterSelectInfoRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            CharacterSelectInfo(app, session);
        } break;
        case LOGIN_CHARACTERLOGINREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterLoginRequest;
            printf(MESSAGE_CONCAT_INFO("Recieved %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterLoginRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            Login_Packet_CharacterLoginReply packetReply = { 0 };

            packetReply.character_id = packet.character_id;
            packetReply.server_id = packet.server_id;
            packetReply.status = 1;

            char ticketBuf[256];
            // Build a Steam64-like identity prefix before ':' so zone can mirror it in SendSelf.
            u64 steamLikeId = 76561197960265728ull + (packet.character_id & 0xffffffffull);
            snprintf(ticketBuf, sizeof(ticketBuf), "%llu:%.*s",
                     (unsigned long long)steamLikeId,
                     (int)session->characterName.size,
                     session->characterName.data);

            packetReply.login_payload = (struct login_payload_s[1]){
                {
                    .server_address = STR8("127.0.0.1:60000"),
                    .server_ticket = string8_make((u8*)ticketBuf, strlen(ticketBuf)),
                    // .server_ticket = STR8("7y3Bh44sKWZCYZH"),
                    .encryption_key =
                        STR8("\x17\xbd\x08\x6b\x1b\x94\xf0\x2f\xf0\xec\x53\xd7\x63\x58\x9b\x5f"),
                    .soe_protocol_version = 3,
                    .character_id = packet.character_id,
                    .character_name = session->characterName,
                },
            };

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterLoginReply, &packetReply);
        } break;
        case 0x6b: {
            printf(MESSAGE_CONCAT_INFO("Received unknown packet 0x6b (ignoring)\n"));
        } break;
        case 0x79: {
            printf(MESSAGE_CONCAT_INFO("Received unknown packet 0x79 (ignoring)\n"));
        } break;
        case 0xb7: {
            printf(MESSAGE_CONCAT_INFO("Received unknown packet 0xb7 (ignoring)\n"));
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled login packet 0x%02x\n"), packetId);
            return;
        }
    }
}