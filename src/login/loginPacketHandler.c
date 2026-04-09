// ============================================================================
// Login Packet Handler
//
// Handles the LoginUdp_11 protocol packets between the client and login server.
// Ref: H1emu/h1z1-server src/servers/LoginServer/loginserver.ts
//
// Flow: LoginRequest → LoginReply → ServerListRequest → ServerListReply
//       → CharacterSelectInfoRequest → CharacterSelectInfoReply
//       → CharacterCreateRequest → CharacterCreateReply (if new character)
//       → CharacterLoginRequest → CharacterLoginReply (connect to zone)
// ============================================================================
void LoginPacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind;
    u8 packetId = *data;
    i32 offset = sizeof(u8);

    switch (packetId) {
        // ================================================================
        // 0x01 LoginRequest — Client sends auth credentials
        // Reply with LoginReply containing session status
        // Ref: H1emu loginserver.ts LoginRequest handler
        // ================================================================
        case LOGIN_LOGINREQUEST_ID: {
            kind = Login_Packet_Kind_LoginRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            // Build a structured LoginReply (replaces raw binary file approach)
            // Ref: H1emu sends LoginReply with status=1, is_member=1
            Login_Packet_LoginReply packetReply = { 0 };
            packetReply.is_logged_in = 1;
            packetReply.status = 1;
            packetReply.result_code = 1;
            packetReply.is_member = 1;
            packetReply.is_internal = 1;

            packetReply.account_features_count = 1;
            packetReply.account_features = (struct account_features_s[1]){
                [0] = { 0 },
            };

            packetReply.error_details_count = 1;
            packetReply.error_details = (struct error_details_s[1]){
                [0] = { 0 },
            };

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_LoginReply, &packetReply);
        } break;

        // ================================================================
        // 0x03 Logout — Client disconnecting from login server
        // ================================================================
        case LOGIN_LOGOUT_ID: {
            kind = Login_Packet_Kind_Logout;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
        } break;

        // ================================================================
        // 0x04 ForceDisconnect — Server-initiated disconnect
        // ================================================================
        case LOGIN_FORCEDISCONNECT_ID: {
            kind = Login_Packet_Kind_ForceDisconnect;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
        } break;

        // ================================================================
        // 0x0D ServerListRequest — Client requests available game servers
        // Reply with ServerListReply containing server entries
        // Ref: H1emu loginserver.ts ServerListRequest handler
        // ================================================================
        case LOGIN_SERVERLISTREQUEST_ID: {
            kind = Login_Packet_Kind_ServerListRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_ServerListReply packetReply = { 0 };

            packetReply.servers_count = 1;
            packetReply.servers = (struct servers_s[1]){{
                .id          = 1,
                .state       = 2,
                .is_locked   = FALSE,
                .name        = STR8("H1Z1-C-Server"),
                .name_id     = 193,
                .description = STR8("A server-emulator for H1Z1: King of the Kill, Preseason 3; Built in C"),
                .description_id = 1362,
                .server_info = STR8(
                    "<ServerInfo Region=\"CharacterCreate.RegionUs\" "
                    "Subregion=\"UI.SubregionUS\" "
                    "IsRecommended=\"1\" IsRecommendedVS=\"0\" "
                    "IsRecommendedNC=\"0\" IsRecommendedTR=\"0\" />"),
                .population_data = STR8(
                    "<Population PctCap=\"0\" PingAdr=\"127.0.0.1:60000\" "
                    "Rulesets=\"\" Mode=\"13\" IsLogin=\"1\" IsWL=\"0\" "
                    "IsEvt=\"0\" PL=\"0\" DC=\"LVS\" PopLock=\"0\" "
                    "GP=\"100\" BP=\"175\" MaxPop=\"4000\" "
                    "Subregion=\"US\"><Fac IsList=\"1\"/></Population>"),
                .is_access_allowed = TRUE,
            }};

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_ServerListReply, &packetReply);
        } break;

        // ================================================================
        // 0x09 CharacterDeleteRequest — Client wants to delete a character
        // Ref: H1emu loginserver.ts CharacterDeleteRequest handler
        // ================================================================
        case LOGIN_CHARACTERDELETEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterDeleteRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterDeleteRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            Login_Packet_CharacterDeleteReply packetReply = { 0 };
            packetReply.character_id = session->characterId;
            packetReply.status = 1;

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterDeleteReply, &packetReply);
        } break;

        // ================================================================
        // 0x11 TunnelAppPacketClientToServer — Character name validation
        // Sent during character creation to validate the chosen name
        // ================================================================
        case LOGIN_TUNNELAPPPACKETCLIENTTOSERVER_ID: {
            NameValidation(app, session, data, dataLen);
        } break;

        // ================================================================
        // 0x05 CharacterCreateRequest — Client creates a new character
        // Extracts head type, creates character, sends reply
        // Ref: H1emu loginserver.ts CharacterCreateRequest handler
        // ================================================================
        case LOGIN_CHARACTERCREATEREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterCreateRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterCreateRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            // Set appearance from head type selection
            SetPlayerActorFromHeadType(session, packet.char_payload->head_type);
            CharacterCreate(app, session);
        } break;

        // ================================================================
        // 0x0B CharacterSelectInfoRequest — Client wants character list
        // Ref: H1emu loginserver.ts CharacterSelectInfoRequest handler
        // ================================================================
        case LOGIN_CHARACTERSELECTINFOREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterSelectInfoRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
            CharacterSelectInfo(app, session);
        } break;

        // ================================================================
        // 0x07 CharacterLoginRequest — Client selects character to play
        // Sends connection info (zone address, ticket, encryption key)
        // Ref: H1emu loginserver.ts CharacterLoginRequest handler
        // ================================================================
        case LOGIN_CHARACTERLOGINREQUEST_ID: {
            kind = Login_Packet_Kind_CharacterLoginRequest;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

            Login_Packet_CharacterLoginRequest packet = { 0 };
            login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

            // Build server ticket with identity:name format
            // Zone server will extract both parts on gateway login
            char ticketBuf[256];
            u64 steamLikeId = 76561197960265728ull + (packet.character_id & 0xffffffffull);
            snprintf(ticketBuf, sizeof(ticketBuf), "%llu:%.*s",
                     (unsigned long long)steamLikeId,
                     (int)session->characterName.size,
                     session->characterName.data);

            Login_Packet_CharacterLoginReply packetReply = { 0 };
            packetReply.character_id = packet.character_id;
            packetReply.server_id = packet.server_id;
            packetReply.status = 1; // 1=ACCEPTED

            packetReply.login_payload = (struct login_payload_s[1]){{
                .server_address  = STR8("127.0.0.1:60000"),
                .server_ticket   = string8_make((u8*)ticketBuf, strlen(ticketBuf)),
                .encryption_key  = STR8("\x17\xbd\x08\x6b\x1b\x94\xf0\x2f\xf0\xec\x53\xd7\x63\x58\x9b\x5f"),
                .soe_protocol_version = 3,
                .character_id    = packet.character_id,
                .character_name  = session->characterName,
            }};

            printf(MESSAGE_CONCAT_INFO("Character 0x%llx '%.*s' connecting to zone server\n"),
                   (unsigned long long)packet.character_id,
                   (int)session->characterName.size, session->characterName.data);

            LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                            Login_Packet_Kind_CharacterLoginReply, &packetReply);
        } break;

        // ================================================================
        // Known but unhandled packets — log and ignore
        // ================================================================
        case 0x6b:
        case 0x79:
        case 0xb7: {
            printf(MESSAGE_CONCAT_INFO("Received known-unhandled packet 0x%02x (ignoring)\n"), packetId);
        } break;

        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled login packet 0x%02x (len=%u)\n"), packetId, dataLen);
        }
    }
}