// ============================================================================
// LoginPacketHandler — Main dispatch for incoming login protocol packets
//
// Based on H1emu/h1z1-server LoginServer packet routing.
// The client sends packets in this typical order:
//   1. LoginRequest (0x01)         → LoginReply
//   2. CharacterSelectInfoReq (0x0b) → CharacterSelectInfoReply
//   3. ServerListRequest (0x0d)     → ServerListReply
//   4. TunnelAppPacket (0x10)       → TunnelAppPacket (name validation)
//   5. CharacterCreateRequest (0x05) → CharacterCreateReply
//   6. CharacterSelectInfoReq (0x0b) → CharacterSelectInfoReply (refresh)
//   7. CharacterLoginRequest (0x07)  → CharacterLoginReply (zone ticket)
//
// Delete flow:
//   CharacterDeleteRequest (0x09)   → CharacterDeleteReply
//
// Session management:
//   Logout (0x03), ForceDisconnect (0x04) — cleanup only
// ============================================================================
void LoginPacketHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind;
    printf("\n");

    u8 packetId = *data;

    switch (packetId) {

        // ====================================================================
        // LoginRequest (0x01) — First packet from client
        // ====================================================================
        case LOGIN_LOGINREQUEST_ID: {
            LoginReplyHandler(app, session, data, dataLen);
        } break;

        // ====================================================================
        // Logout (0x03) — Client gracefully disconnecting from login
        // ====================================================================
        case LOGIN_LOGOUT_ID: {
            kind = Login_Packet_Kind_Logout;
            printf(MESSAGE_CONCAT_INFO("Received %s — session cleanup\n"), login_packet_names[kind]);
        } break;

        // ====================================================================
        // ForceDisconnect (0x04) — Server-initiated disconnect
        // ====================================================================
        case LOGIN_FORCEDISCONNECT_ID: {
            kind = Login_Packet_Kind_ForceDisconnect;
            printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);
        } break;

        // ====================================================================
        // CharacterCreateRequest (0x05)
        // ====================================================================
        case LOGIN_CHARACTERCREATEREQUEST_ID: {
            CharacterCreateHandler(app, session, data, dataLen);
        } break;

        // ====================================================================
        // CharacterLoginRequest (0x07) — Launch into zone server
        // ====================================================================
        case LOGIN_CHARACTERLOGINREQUEST_ID: {
            CharacterLoginHandler(app, session, data, dataLen);
        } break;

        // ====================================================================
        // CharacterDeleteRequest (0x09)
        // ====================================================================
        case LOGIN_CHARACTERDELETEREQUEST_ID: {
            CharacterDeleteHandler(app, session, data, dataLen);
        } break;

        // ====================================================================
        // CharacterSelectInfoRequest (0x0b)
        // ====================================================================
        case LOGIN_CHARACTERSELECTINFOREQUEST_ID: {
            CharacterSelectInfoHandler(app, session);
        } break;

        // ====================================================================
        // ServerListRequest (0x0d)
        // ====================================================================
        case LOGIN_SERVERLISTREQUEST_ID: {
            ServerListReplyHandler(app, session);
        } break;

        // ====================================================================
        // TunnelAppPacketClientToServer (0x10) — Name validation
        // ====================================================================
        case LOGIN_TUNNELAPPPACKETCLIENTTOSERVER_ID: {
            NameValidation(app, session, data, dataLen);
        } break;

        // ====================================================================
        // Known but unhandled packets — logged but no action taken
        // ====================================================================
        case 0x6b:
        case 0x79:
        case 0xb7: {
            printf(MESSAGE_CONCAT_INFO("Received known-unhandled login packet 0x%02x (ignoring)\n"),
                   packetId);
        } break;

        // ====================================================================
        // Unknown packets
        // ====================================================================
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled login packet 0x%02x (len=%u)\n"),
                   packetId, dataLen);
        }
    }
}