// ============================================================================
// Gateway Protocol — Pack / Unpack / Handle
//
// The gateway layer sits between SOE transport and zone client protocol.
// It handles: LoginRequest, LoginReply, TunnelPacket routing, ChannelIsRoutable.
//
// Ref: H1emu/h1z1-server src/servers/GatewayServer/gatewayserver.ts
//
// Channels:
//   0 = main zone protocol (encrypted after login)
//   1 = secondary channel (routable)
//   2 = position updates (silently dropped for now)
//   4,5 = additional channels (routable)
// ============================================================================

u32 GatewayPacketPack(GatewayKindEnum kind, void* packetPtr, u8* buffer) {
    u32 offset = 0;

    switch (kind) {
        case GatewayKindLoginRequest: {
            GatewayLoginRequest* packet = packetPtr;

            endian_write_u8_little(buffer + offset, GatewayLoginRequestId);
            offset++;

            endian_write_u64_little(buffer + offset, packet->characterId);
            offset += sizeof(u64);

            endian_write_u32_little(buffer + offset, packet->serverTicketLen);
            offset += sizeof(u32);

            for (u32 i = 0; i < packet->serverTicketLen; i++) {
                endian_write_i8_little(buffer + offset, packet->serverTicket[i]);
                offset++;
            }

            endian_write_u32_little(buffer + offset, packet->clientProtocolLen);
            offset += sizeof(u32);

            for (u32 i = 0; i < packet->clientProtocolLen; i++) {
                endian_write_i8_little(buffer + offset, packet->clientProtocol[i]);
                offset++;
            }

            endian_write_u32_little(buffer + offset, packet->clientBuildLen);
            offset += sizeof(u32);

            for (u32 i = 0; i < packet->clientBuildLen; i++) {
                endian_write_i8_little(buffer + offset, packet->clientBuild[i]);
                offset++;
            }
        } break;
        case GatewayKindLoginReply: {
            GatewayLoginReply* packet = packetPtr;

            endian_write_u8_little(buffer + offset, GatewayLoginReplyId);
            offset++;

            endian_write_b8_little(buffer + offset, packet->isLoggedIn);
            offset += sizeof(b8);
        } break;
        case GatewayKindTunnelPacketToExternalConnection:
        case GatewayKindTunnelPacketFromExternalConnection: {
            GatewayTunnelPacket* packet = packetPtr;

            u8 opcode;
            if (kind == GatewayKindTunnelPacketToExternalConnection) {
                opcode = GatewayTunnelToExternalConnectionId;
            } else if (kind == GatewayKindTunnelPacketFromExternalConnection) {
                opcode = GatewayTunnelFromExternalConnectionId;
            } else {
                opcode = 0;
                ABORT_MSG("Packing unknown TunnelPacket");
            }

            endian_write_u8_little(buffer + offset, opcode | (packet->channel << 5));
            offset++;

            memcpy(buffer + offset, packet->data, packet->dataLen);
            offset += packet->dataLen;
        } break;
        case GatewayKindChannelIsRoutable: {
            GatewayChannelIsRoutable* packet = packetPtr;

            endian_write_u8_little(buffer + offset,
                                   GatewayChannelIsRoutableId | (packet->channel << 5));
            offset++;

            endian_write_b8_little(buffer + offset, packet->isRoutable);
            offset += sizeof(b8);

            endian_write_b8_little(buffer + offset, packet->unkBool);
            offset += sizeof(b8);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Packing %s not implemented\n"), gatewayKindNames[kind]);
        }
    }

    return offset;
}

void GatewayPacketUnpack(u8* data, u32 dataLen, GatewayKindEnum kind, void* packetPtr, Arena* arena) {
    u32 offset = 1;

    switch (kind) {
        case GatewayKindLoginRequest: {
            GatewayLoginRequest* packet = packetPtr;

            packet->characterId = endian_read_u64_little(data + offset);
            offset += sizeof(u64);

            packet->serverTicketLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->serverTicket = arena_push_size(arena, packet->serverTicketLen);
            for (u32 i = 0; i < packet->serverTicketLen; i++) {
                packet->serverTicket[i] = *(i8*)((uptr)data + offset);
                offset++;
            }

            packet->clientProtocolLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->clientProtocol = arena_push_size(arena, packet->clientProtocolLen);
            for (u32 i = 0; i < packet->clientProtocolLen; i++) {
                packet->clientProtocol[i] = *(i8*)((uptr)data + offset);
                offset++;
            }

            packet->clientBuildLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->clientBuild = arena_push_size(arena, packet->clientBuildLen);
            for (u32 i = 0; i < packet->clientBuildLen; i++) {
                packet->clientBuild[i] = *(i8*)((uptr)data + offset);
                offset++;
            }
        } break;
        case GatewayKindLoginReply: {
            GatewayLoginReply* packet = packetPtr;

            packet->isLoggedIn = endian_read_b8_little(data + offset);
            offset += sizeof(b8);
        } break;
        case GatewayKindTunnelPacketFromExternalConnection: {
            GatewayTunnelPacket* packet = packetPtr;

            packet->channel = (*data) >> 5;
            packet->data = data + offset;
            packet->dataLen = dataLen - 1;
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Unpacking %s not implemented\n"), gatewayKindNames[kind]);
        }
    }
}

void GatewayTunnelDataSend(AppState* app, SessionState* session, u8* baseBuffer, u32 totalLen) {
    GatewayTunnelPacket tunnelPacket = {
        .channel = 0,
        .data = baseBuffer + TunnelDataHeaderLen,
        .dataLen = totalLen - TunnelDataHeaderLen,
    };

    u32 packedLen =
        GatewayPacketPack(GatewayKindTunnelPacketToExternalConnection, &tunnelPacket, baseBuffer);
    OutputStreamWrite(app, session, &session->outputStream, baseBuffer, packedLen, FALSE);
}

void GatewayPacketSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                       GatewayKindEnum kind, void* packetPtr) {
    u8* packedBuffer = arena_push_size(arena, maxLen);
    u32 packedLen = GatewayPacketPack(kind, packetPtr, packedBuffer);

    OutputStreamWrite(app, session, &session->outputStream, packedBuffer, packedLen, FALSE);
}

// ============================================================================
// Extract character name and identity from server ticket.
// Ticket format: "76561197960265729:CharacterName"
// Before ':' = Steam-like identity, after ':' = character name.
// Stores into session using arenaTotal so it persists across ticks.
// ============================================================================
void GatewayExtractCharacterName(AppState* app, SessionState* session,
                                 char* ticket, u32 ticketLen) {
    u32 colonPos = 0;
    b32 found = FALSE;
    for (u32 i = 0; i < ticketLen; i++) {
        if (ticket[i] == ':') {
            colonPos = i;
            found = TRUE;
            break;
        }
    }

    if (found && colonPos > 0) {
        session->ticketIdentity.size = colonPos;
        session->ticketIdentity.data = arena_push_size(&app->arenaTotal, colonPos);
        memcpy(session->ticketIdentity.data, ticket, colonPos);
    } else {
        session->ticketIdentity.size = 0;
        session->ticketIdentity.data = NULL;
    }

    if (found && (colonPos + 1) < ticketLen) {
        u32 nameLen = ticketLen - colonPos - 1;
        session->characterName.size = nameLen;
        session->characterName.data = arena_push_size(&app->arenaTotal, nameLen);
        memcpy(session->characterName.data, ticket + colonPos + 1, nameLen);

        printf(MESSAGE_CONCAT_INFO("Ticket: identity='%.*s' name='%.*s'\n"),
               (int)session->ticketIdentity.size, session->ticketIdentity.data,
               (int)nameLen, session->characterName.data);
    } else {
        session->characterName.size = 0;
        session->characterName.data = NULL;
        printf(MESSAGE_CONCAT_WARN("No character name found in server ticket\n"));
    }
}

// ============================================================================
// GatewayPacketHandle — Routes incoming gateway-level packets.
//
// Ref: H1emu/h1z1-server src/servers/GatewayServer/gatewayserver.ts
//
// LoginRequest → enable encryption → LoginReply → ChannelIsRoutable × 5
//             → GatewayOnLogin (triggers zone init)
// TunnelPacket → extract channel/data → ZonePacketHandler
// ============================================================================
void GatewayPacketHandle(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    GatewayKindEnum kind;

    u8 channel  = *data >> 5;
    u8 packetId = *data & 0b00011111;

    // Non-zero channel = tunnel data on alternate channel
    if (channel != 0) {
        if (channel == 2) {
            return; // position updates — silently drop
        }
        if (dataLen > 1) {
            GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
        }
        return;
    }

    switch (packetId) {
        // ================================================================
        // Gateway LoginRequest — client authenticating to zone
        // Enable encryption, send LoginReply, mark channels routable,
        // then trigger zone OnLogin
        // ================================================================
        case GatewayLoginRequestId: {
            kind = GatewayKindLoginRequest;
            printf(MESSAGE_CONCAT_INFO("Gateway LoginRequest\n"));

            GatewayLoginRequest loginRequest = { 0 };
            GatewayPacketUnpack(data, dataLen, kind, &loginRequest, &app->arenaPerTick);

            GatewayExtractCharacterName(app, session,
                                        loginRequest.serverTicket,
                                        loginRequest.serverTicketLen);

            if (!session->isLoggedIn) {
                // Enable encryption on main channel streams
                session->inputStream.useEncryption  = TRUE;
                session->outputStream.useEncryption = TRUE;

                // Initialize RC4 for secondary channels
                session->inputStream1.useEncryption = TRUE;
                crypt_rc4_initialize(&session->inputStream1.rc4, app->rc4Decoded, app->rc4DecodedLen);
                session->inputStream2.useEncryption = TRUE;
                crypt_rc4_initialize(&session->inputStream2.rc4, app->rc4Decoded, app->rc4DecodedLen);
                session->inputStream4.useEncryption = TRUE;
                crypt_rc4_initialize(&session->inputStream4.rc4, app->rc4Decoded, app->rc4DecodedLen);
                session->inputStream5.useEncryption = TRUE;
                crypt_rc4_initialize(&session->inputStream5.rc4, app->rc4Decoded, app->rc4DecodedLen);
            }

            session->isLoggedIn = TRUE;

            GatewayLoginReply loginReply = {
                .isLoggedIn = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindLoginReply, &loginReply);

            // Mark all channels as routable
            // Ref: H1emu gatewayserver.ts sends ChannelIsRoutable for ch 0,1,2,4,5
            u8 routableChannels[] = { 0, 1, 2, 4, 5 };
            for (u32 ch = 0; ch < sizeof(routableChannels); ch++) {
                GatewayChannelIsRoutable chRoutable = {
                    .channel = routableChannels[ch], .isRoutable = TRUE, .unkBool = TRUE,
                };
                GatewayPacketSend(app, session, &app->arenaPerTick, 32,
                                  GatewayKindChannelIsRoutable, &chRoutable);
            }

            GatewayOnLogin(app, session, loginRequest.characterId);
        } break;
        // ================================================================
        // TunnelPacket — zone protocol data from client
        // ================================================================
        case GatewayTunnelFromExternalConnectionId: {
            GatewayTunnelPacket tunnelPacket = { 0 };
            GatewayPacketUnpack(data, dataLen, GatewayKindTunnelPacketFromExternalConnection,
                                &tunnelPacket, &app->arenaPerTick);
            GatewayOnTunnelDataFromClient(app, session, tunnelPacket.data, tunnelPacket.dataLen);
        } break;

        // ================================================================
        // Alternate channel routing (0x09, 0x0a, 0x18, 0x19)
        // ================================================================
        case 0x18:
        case 0x09:
        case 0x0a:
        case 0x19: {
            if (dataLen > 1) {
                GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
            }
        } break;
        default: {
            if (dataLen > 1) {
                GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
            } else {
                printf(MESSAGE_CONCAT_WARN("Unhandled gateway packet 0x%02x\n"), packetId);
            }
        }
    }
}