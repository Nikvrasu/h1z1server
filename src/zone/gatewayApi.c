// ============================================================================
// Gateway Protocol Layer — Routes packets between SOE protocol and zone server
//
// Based on H1emu/h1z1-server GatewayServer implementation.
//
// The gateway sits between the SOE transport layer and the game protocol:
//   SOE Protocol (UDP) → Gateway → Zone/Login Packet Handler
//
// Gateway packet types:
//   LoginRequest (0x01)  — Client authenticates with character_id + server_ticket
//   LoginReply (0x02)    — Server confirms login, enables encryption
//   TunnelPacketTo (0x05)   — Server → Client tunnel data (zone packets)
//   TunnelPacketFrom (0x06) — Client → Server tunnel data (zone packets)
//   ChannelIsRoutable (0x07) — Declare channels 0-5 as routable
//
// Channel routing:
//   Channel 0 = Main game data (zone packets)
//   Channel 1 = Secondary data
//   Channel 2 = Position updates (dropped for now)
//   Channel 4 = Auxiliary data
//   Channel 5 = Auxiliary data
//
// Login flow:
//   1. Client sends LoginRequest with characterId, serverTicket, clientProtocol
//   2. Server extracts character name from ticket (format: "steamId:charName")
//   3. Server enables RC4 encryption on all channels
//   4. Server replies with LoginReply + ChannelIsRoutable for channels 0-5
//   5. Server triggers OnLogin to start the zone initialization sequence
// ============================================================================

u32 GatewayPacketPack(GatewayKindEnum kind, void* packetPtr, u8* buffer) {
    u32 offset = 0;

    switch (kind) {
        case GatewayKindLoginRequest: {
            printf(MESSAGE_CONCAT_INFO("Packing LoginRequest...\n"));
            GatewayLoginRequest* packet = packetPtr;

            endian_write_u8_little(buffer + offset, GatewayLoginRequestId);
            offset++;

            endian_write_u64_little(buffer + offset, packet->characterId);
            offset += sizeof(u64);
            printf("-- characterId            \t%lld\t%llxh\t%f\n", (i64)packet->characterId,
                   (u64)packet->characterId, (f64)packet->characterId);

            endian_write_u32_little(buffer + offset, packet->serverTicketLen);
            offset += sizeof(u32);
            printf("-- serverTicketLen           \t%lld\t%llxh\t%f\n", (i64)packet->serverTicketLen,
                   (u64)packet->serverTicketLen, (f64)packet->serverTicketLen);

            for (u32 serverTickeIter = 0; serverTickeIter < packet->serverTicketLen;
                 serverTickeIter++) {
                endian_write_i8_little(buffer + offset, packet->serverTicket[serverTickeIter]);
                offset++;
            }

            endian_write_u32_little(buffer + offset, packet->clientProtocolLen);
            offset += sizeof(u32);
            printf("-- clientProtocolLen           \t%lld\t%llxh\t%f\n", (i64)packet->clientProtocolLen,
                   (u64)packet->clientProtocolLen, (f64)packet->clientProtocolLen);

            for (u32 clientProtocolIter = 0; clientProtocolIter < packet->clientProtocolLen;
                 clientProtocolIter++) {
                endian_write_i8_little(buffer + offset, packet->clientProtocol[clientProtocolIter]);
                offset++;
            }

            endian_write_u32_little(buffer + offset, packet->clientBuildLen);
            offset += sizeof(u32);
            printf("-- clientBuildLen           \t%lld\t%llxh\t%f\n", (i64)packet->clientBuildLen,
                   (u64)packet->clientBuildLen, (f64)packet->clientBuildLen);

            for (u32 clientBuildIter = 0; clientBuildIter < packet->clientBuildLen; clientBuildIter++) {
                endian_write_i8_little(buffer + offset, packet->clientBuild[clientBuildIter]);
                offset++;
            }
        } break;
        case GatewayKindLoginReply: {
            printf(MESSAGE_CONCAT_INFO("Packing LoginReply...\n"));
            GatewayLoginReply* packet = packetPtr;

            endian_write_u8_little(buffer + offset, GatewayLoginReplyId);
            offset++;

            endian_write_b8_little(buffer + offset, packet->isLoggedIn);
            offset += sizeof(b8);

            printf("-- isLoggedIn            \t%lld\t%llxh\t%f\n", (i64)packet->isLoggedIn,
                   (u64)packet->isLoggedIn, (f64)packet->isLoggedIn);
        } break;
        case GatewayKindTunnelPacketToExternalConnection:
        case GatewayKindTunnelPacketFromExternalConnection: {
            printf(MESSAGE_CONCAT_INFO("Packing %s...\n"), gatewayKindNames[kind]);
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

            printf("-- channel                 \t%lld\t%llxh\t%f\n", (i64)packet->channel,
                   (u64)packet->channel, (f64)packet->channel);
            memcpy(buffer + offset, packet->data, packet->dataLen);
            offset += packet->dataLen;
        } break;
        case GatewayKindChannelIsRoutable: {
            printf(MESSAGE_CONCAT_INFO("Packing ChannelIsRoutable...\n"));
            GatewayChannelIsRoutable* packet = packetPtr;

            endian_write_u8_little(buffer + offset,
                                   GatewayChannelIsRoutableId | (packet->channel << 5));
            offset++;

            printf("-- channel                 \t%lld\t%llxh\t%f\n", (i64)packet->channel,
                   (u64)packet->channel, (f64)packet->channel);

            endian_write_b8_little(buffer + offset, packet->isRoutable);
            offset += sizeof(b8);

            printf("-- isRoutable             \t%lld\t%llxh\t%f\n", (i64)packet->isRoutable,
                   (u64)packet->isRoutable, (f64)packet->isRoutable);

            endian_write_b8_little(buffer + offset, packet->unkBool);
            offset += sizeof(b8);

            printf("-- unkBool                \t%lld\t%llxh\t%f\n", (i64)packet->unkBool,
                   (u64)packet->unkBool, (f64)packet->unkBool);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Packing %s not implemented\n"), gatewayKindNames[kind]);
        }
    }

    return offset;
}

void GatewayPacketUnpack(u8* data, u32 dataLen, GatewayKindEnum kind, void* packetPtr, Arena* arena) {
    u32 offset = 1;
    printf("\n");

    switch (kind) {
        case GatewayKindLoginRequest: {
            printf(MESSAGE_CONCAT_INFO("Unpacking LoginRequest...\n"));
            GatewayLoginRequest* packet = packetPtr;

            packet->characterId = endian_read_u64_little(data + offset);
            offset += sizeof(u64);

            printf("-- characterId            \t%lld\t%llxh\t%f\n", (i64)packet->characterId,
                   (u64)packet->characterId, (f64)packet->characterId);

            packet->serverTicketLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->serverTicket = arena_push_size(arena, packet->serverTicketLen);
            printf("-- serverTicketLen           \t%d\n", packet->serverTicketLen);

            for (u32 serverTicketIter = 0; serverTicketIter < packet->serverTicketLen;
                 serverTicketIter++) {
                packet->serverTicket[serverTicketIter] = *(i8*)((uptr)data + offset);
                offset++;
            }

            packet->clientProtocolLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->clientProtocol = arena_push_size(arena, packet->clientProtocolLen);
            printf("-- clientProtocolLen           \t%d\n", packet->clientProtocolLen);

            for (u32 clientProtocolIter = 0; clientProtocolIter < packet->clientProtocolLen;
                 clientProtocolIter++) {
                packet->clientProtocol[clientProtocolIter] = *(i8*)((uptr)data + offset);
                offset++;
            }

            packet->clientBuildLen = endian_read_u32_little(data + offset);
            offset += sizeof(u32);

            packet->clientBuild = arena_push_size(arena, packet->clientBuildLen);
            printf("-- clientBuildLen           \t%d\n", packet->clientBuildLen);

            for (u32 clientBuildIter = 0; clientBuildIter < packet->clientBuildLen; clientBuildIter++) {
                packet->clientBuild[clientBuildIter] = *(i8*)((uptr)data + offset);
                offset++;
            }
        } break;
        case GatewayKindLoginReply: {
            printf(MESSAGE_CONCAT_INFO("Unpacking LoginReply...\n"));
            GatewayLoginReply* packet = packetPtr;

            packet->isLoggedIn = endian_read_b8_little(data + offset);
            offset += sizeof(b8);

            printf("-- isLoggedIn            \t%lld\t%llxh\t%f\n", (i64)packet->isLoggedIn,
                   (u64)packet->isLoggedIn, (f64)packet->isLoggedIn);
        } break;
        case GatewayKindTunnelPacketFromExternalConnection: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), gatewayKindNames[kind]);
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
// GatewayExtractCharacterName — Extract identity and name from server ticket
//
// H1emu ticket format: "steamId64:CharacterName"
//   - Everything before the first ':' is the ticket identity (Steam64-like ID)
//   - Everything after is the character name
//   - Both are stored in session for use by SendSelfToClient
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
        printf("[GW] Extracted ticket identity: '%.*s' len=%u\n",
               (int)session->ticketIdentity.size, session->ticketIdentity.data,
               (u32)session->ticketIdentity.size);
    } else {
        session->ticketIdentity.size = 0;
        session->ticketIdentity.data = NULL;
        printf("[GW] WARNING: No ticket identity found in server ticket!\n");
    }

    if (found && (colonPos + 1) < ticketLen) {
        u32 nameLen = ticketLen - colonPos - 1;
        session->characterName.size = nameLen;
        session->characterName.data = arena_push_size(&app->arenaTotal, nameLen);
        memcpy(session->characterName.data, ticket + colonPos + 1, nameLen);

        printf("[GW] Extracted character name from ticket: '%.*s' len=%u\n",
               (int)nameLen, session->characterName.data, nameLen);
    } else {
        printf("[GW] WARNING: No character name found in server ticket!\n");
        session->characterName.size = 0;
        session->characterName.data = NULL;
    }
}

void GatewayPacketHandle(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    printf("[GW] Raw byte 0x%02x, dataLen=%u\n", data[0], dataLen);
    GatewayKindEnum kind;
    printf("\n");

    u8 channel  = *data >> 5;
    u8 packetId = *data & 0b00011111;

    if (channel != 0) {
        if (channel == 2) {
            // Channel 2 = position updates, silently drop for now
            return;
        }
        printf(MESSAGE_CONCAT_INFO("(%u) Routing channel %u data as tunnel data\n"), channel, channel);
        if (dataLen > 1) {
            GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
        }
        return;
    }

    switch (packetId) {
        case GatewayLoginRequestId: {
            kind = GatewayKindLoginRequest;
            printf(MESSAGE_CONCAT_INFO("(%u) Handling %s...\n"), channel, gatewayKindNames[kind]);

            GatewayLoginRequest loginRequest = { 0 };
            GatewayPacketUnpack(data, dataLen, kind, &loginRequest, &app->arenaPerTick);

            GatewayExtractCharacterName(app, session,
                                        loginRequest.serverTicket,
                                        loginRequest.serverTicketLen);

            if (!session->isLoggedIn) {
                printf("[*] Enabling encryption for session (first login)\n");

                // Enable encryption on ch0 streams — do NOT re-init their RC4,
                // the keystream position must be preserved from session creation.
                session->inputStream.useEncryption  = TRUE;
                session->outputStream.useEncryption = TRUE;

                // Ch1/2/4/5 haven't been used yet so init their RC4 now.
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

            GatewayChannelIsRoutable channelZeroIsRoutable = {
                .channel = 0, .isRoutable = TRUE, .unkBool = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindChannelIsRoutable,
                            &channelZeroIsRoutable);

            GatewayChannelIsRoutable channelOneIsRoutable = {
                .channel = 1, .isRoutable = TRUE, .unkBool = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindChannelIsRoutable,
                            &channelOneIsRoutable);

            GatewayChannelIsRoutable channelTwoIsRoutable = {
                .channel = 2, .isRoutable = TRUE, .unkBool = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindChannelIsRoutable,
                            &channelTwoIsRoutable);

            GatewayChannelIsRoutable channelFourIsRoutable = {
                .channel = 4, .isRoutable = TRUE, .unkBool = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindChannelIsRoutable,
                            &channelFourIsRoutable);

            GatewayChannelIsRoutable channelFiveIsRoutable = {
                .channel = 5, .isRoutable = TRUE, .unkBool = TRUE,
            };
            GatewayPacketSend(app, session, &app->arenaPerTick, 32, GatewayKindChannelIsRoutable,
                            &channelFiveIsRoutable);

            GatewayOnLogin(app, session, loginRequest.characterId);
        } break;
        case GatewayTunnelFromExternalConnectionId: {
            kind = GatewayKindTunnelPacketFromExternalConnection;
            printf(MESSAGE_CONCAT_INFO("(%u) Handling %s...\n"), channel, gatewayKindNames[kind]);

            GatewayTunnelPacket tunnelPacket = { 0 };
            GatewayPacketUnpack(data, dataLen, kind, &tunnelPacket, &app->arenaPerTick);

            GatewayOnTunnelDataFromClient(app, session, tunnelPacket.data, tunnelPacket.dataLen);
        } break;
        case 0x18:
        case 0x09:
        case 0x0a:
        case 0x19: {
            printf(MESSAGE_CONCAT_INFO("(%u) Routing alternate channel 0x%02x as tunnel data\n"), channel, packetId);
            if (dataLen > 1) {
                GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
            }
        } break;
        default: {
            if (dataLen > 1) {
                printf(MESSAGE_CONCAT_INFO("(%u) Routing 0x%02x as tunnel data\n"),
                       channel, packetId);
                GatewayOnTunnelDataFromClient(app, session, data + 1, dataLen - 1);
            }
        }
    }
}