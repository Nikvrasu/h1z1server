u32 CorePacketPack(CoreKindEnum kind, void* packetPtr, u8* buffer, b32 isSubPacket,
                   ConnectionArgs* args) {
    u32 offset = 0;
    printf("\n");

    switch (kind) {
        case CoreKindSessionReply: {
            printf(MESSAGE_CONCAT_INFO("Packing %s...\n"), coreKindNames[kind]);
            SessionReply* packet = packetPtr;

            endian_write_u16_big(buffer + offset, CoreSessionReplyId);
            offset += 2;
            endian_write_u32_big(buffer + offset, packet->sessionId);
            offset += 4;
            endian_write_u32_big(buffer + offset, packet->crcSeed);
            offset += 4;
            *(u8*)((uptr)buffer + offset) = packet->crcLen;
            offset += 1;
            endian_write_u16_big(buffer + offset, packet->compression);
            offset += 2;
            endian_write_u32_big(buffer + offset, packet->udpLen);
            offset += 4;
            endian_write_u32_big(buffer + offset, packet->soeProtocolVersion);
            offset += 4;

            printf("--- sessionId: %d\n", packet->sessionId);
            printf("--- crcSeed: %d\n", packet->crcSeed);
            printf("--- crcLen: %d\n", packet->crcLen);
            printf("--- compression: %d\n", packet->compression);
            printf("--- udpLen: %d\n", packet->udpLen);
            printf("--- soeProtocolVersion: %d\n", packet->soeProtocolVersion);
        } break;
        case CoreKindData: {
            printf(MESSAGE_CONCAT_INFO("Packing %s...\n"), coreKindNames[kind]);
            Data* packet = packetPtr;

            endian_write_u16_big(buffer + offset, CoreDataId);
            offset += 2;

            if (args->compression) {
                offset++;
            }

            endian_write_u16_big(buffer + offset, packet->sequence);
            offset += 2;

            memcpy(buffer + offset, packet->data, packet->dataLen);
            offset += packet->dataLen;

            if (!isSubPacket && args->crcLen) {
                offset += args->crcLen;
            }
        } break;
        case CoreKindDataFragment: {
            printf(MESSAGE_CONCAT_INFO("Packing %s...\n"), coreKindNames[kind]);
            Data* packet = packetPtr;

            endian_write_u16_big(buffer + offset, CoreDataFragmentId);
            offset += 2;

            if (args->compression) {
                offset++;
            }

            endian_write_u16_big(buffer + offset, packet->sequence);
            offset += 2;

            memcpy(buffer + offset, packet->data, packet->dataLen);
            offset += packet->dataLen;

            if (!isSubPacket && args->crcLen) {
                offset += args->crcLen;
            }
        } break;
        case CoreKindAck: {
            printf(MESSAGE_CONCAT_INFO("Packing %s...\n"), coreKindNames[kind]);
            Ack* packet = packetPtr;

            u16 ackId = (u16)(CoreAckId + packet->channel * CoreChannelStride);
            endian_write_u16_big(buffer + offset, ackId);
            offset += 2;
            endian_write_u16_big(buffer + offset, packet->sequence);
            offset += 2;

            printf("--- channel: %d, sequence: %d\n", packet->channel, packet->sequence);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Packing %s not implemented\n"), coreKindNames[kind]);
        }
    }

    return offset;
}

void CorePacketUnpack(u8* data, i32 dataLen, CoreKindEnum kind, void* packetPtr, b32 isSubPacket,
                      ConnectionArgs* args) {
    u32 offset = 2;
    printf("\n");

    switch (kind) {
        case CoreKindSessionRequest: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), coreKindNames[kind]);
            SessionRequest* packet = packetPtr;

            packet->crcLen = endian_read_u32_big(data + offset);
            offset += 4;
            packet->sessionId = endian_read_u32_big(data + offset);
            offset += 4;
            packet->udpLen = endian_read_u32_big(data + offset);
            offset += 4;

            for (i32 i = 0; i < 32; i++) {
                if (!*(u8*)(data + offset + i)) {
                    break;
                }

                packet->protocolName[i] = *(u8*)(data + offset + i);
            }

            printf("%-16s %-20d\n", "crcLen", packet->crcLen);
            printf("%-16s %-20d\n", "sessionId", packet->sessionId);
            printf("%-16s %-20d\n", "udpLength", packet->udpLen);
            printf("%-16s %-20s\n", "protocolName", packet->protocolName);
        } break;
        case CoreKindSessionReply: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), coreKindNames[kind]);
            SessionReply* packet = packetPtr;

            packet->sessionId = endian_read_u32_big(data + offset);
            offset += 4;
            packet->crcSeed = endian_read_u32_big(data + offset);
            offset += 4;
            packet->crcLen = *(u8*)(data + offset);
            offset += 1;
            packet->compression = (u8)endian_read_u16_big(data + offset);
            offset += 2;
            packet->udpLen = endian_read_u32_big(data + offset);
            offset += 4;
            packet->soeProtocolVersion = endian_read_u32_big(data + offset);
            offset += 4;

            printf("%-16s %-24d\n", "sessionId", packet->sessionId);
            printf("%-16s %-24d\n", "crcSeed", packet->crcSeed);
            printf("%-16s %-24d\n", "crcLen", packet->crcLen);
            printf("%-16s %-24d\n", "compression", packet->compression);
            printf("%-16s %-24d\n", "udpLen", packet->udpLen);
            printf("%-16s %-24d\n", "soeProtocolVersion", packet->soeProtocolVersion);
        } break;
        case CoreKindData: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), coreKindNames[kind]);
            Data* packet = packetPtr;

            if (args->compression && !isSubPacket) {
                offset++;
            }

            packet->sequence = endian_read_u16_big(data + offset);
            offset += 2;

            u32 dataEnd;

            if (isSubPacket) {
                dataEnd = dataLen;
            } else {
                dataEnd = dataLen - args->crcLen;
            }

            packet->data = (u8*)((uptr)data + offset);
            packet->dataLen = dataEnd - offset;

            printf("%-16s %-20d\n", "sequence", packet->sequence);
            printf("%-16s %-20p\n", "data", packet->data);
            printf("%-16s %-20d\n", "dataLen", packet->dataLen);
            printf("%-16s %-20x\n", "crc", packet->crc);
        } break;
        case CoreKindDataFragment: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), coreKindNames[kind]);
            Data* packet = packetPtr;

            if (args->compression && !isSubPacket) {
                offset++;
            }

            packet->sequence = endian_read_u16_big(data + offset);
            offset += 2;

            u32 fragmentEnd;
            if (isSubPacket) {
                fragmentEnd = dataLen;
            } else {
                fragmentEnd = dataLen - args->crcLen;
            }

            packet->data = (u8*)((uptr)data + offset);
            packet->dataLen = fragmentEnd - offset;

            printf("%-16s %-20d\n", "sequence", packet->sequence);
            printf("%-16s %-20p\n", "data", packet->data);
            printf("%-16s %-20d\n", "dataLen", packet->dataLen);
            printf("%-16s %-20x\n", "crc", packet->crc);
        } break;
        case CoreKindAck: {
            printf(MESSAGE_CONCAT_INFO("Unpacking %s...\n"), coreKindNames[kind]);
            Ack* packet = packetPtr;

            if (args->compression && !isSubPacket) {
                offset++;
            }

            packet->sequence = endian_read_u16_big(data + offset);
            offset += 2;

            printf("--- sequence: %d\n", packet->sequence);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Unpacking %s not implemented\n"), coreKindNames[kind]);
        }
    }
}

void CorePacketSend(PlatformSocket socket, PlatformApi* api, u32 ip, u16 port, ConnectionArgs* args,
                    CoreKindEnum kind, void* packetPtr) {
    u8 buffer[MAX_PACKET_LENGTH] = { 0 };
    u32 packedLen;

    switch (kind) {
        case CoreKindSessionReply: {
            packedLen = CorePacketPack(kind, packetPtr, buffer, FALSE, args);
        } break;
        case CoreKindData: {
            packedLen = CorePacketPack(kind, packetPtr, buffer, FALSE, args);
        } break;
        case CoreKindDataFragment: {
            packedLen = CorePacketPack(kind, packetPtr, buffer, FALSE, args);
        } break;
        case CoreKindAck: {
            packedLen = CorePacketPack(kind, packetPtr, buffer, FALSE, args);
        } break;
        default: {
            ABORT_MSG("Unable to send unhandled packet!\n");
            return;
        }
    }

    u32 sentLen = api->send_to(socket, buffer, packedLen, ip, port);

    if (!sentLen) {
        printf(MESSAGE_CONCAT_WARN("Sent packet length is 0\n"));
        return;
    }

    printf("\n");
    printf(MESSAGE_CONCAT_INFO("Sent %d bytes to %u.%u.%u.%u:%u\n"), sentLen, (ip & 0xff000000) >> 24,
           (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff), port);
}

CoreKindEnum CorePacketGetKind(u8* data, u32 dataLen) {
    UNUSED(dataLen);
    u16 packetId = endian_read_u16_big(data);

    switch (packetId) {
        case CoreSessionRequestId: {
            return CoreKindSessionRequest;
        }
    }

    return CoreKindUnhandled;
}

void CorePacketHandle(AppState* app, SessionState* session, PlatformApi* api, u8* data, u32 dataLen,
                      b32 isSubPacket) {

        printf("[CORE HANDLE] opcode=0x%04x dataLen=%u isSubPacket=%d\n", 
           endian_read_u16_big(data), dataLen, isSubPacket);
    CoreKindEnum kind;
    u32 offset;

    printf("\n");
    u16 packetId = endian_read_u16_big(data);

    if (isSubPacket) {
        printf("isSubPacket: %d\n", isSubPacket);
    }

    switch (packetId) {
        case CoreSessionRequestId: {
            kind = CoreKindSessionRequest;
            printf(MESSAGE_CONCAT_INFO("Handling %s...\n"), coreKindNames[kind]);

            SessionRequest packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &app->args);

            if (!packet.protocolName[0]) {
                printf(MESSAGE_CONCAT_INFO("Switching to Ping Responder"));
                session->inputStream.dataCallbackPtr = &app->streamFunctionTable->pingInputData;
                session->kind = SessionKindPingResponder;
            }

            // If this is a reconnect, reset all stream state
            if (session->isLoggedIn) {
                printf("[*] Session reconnect — resetting all stream state\n");
                session->isLoggedIn = FALSE;

                // Reset sequence counters
                session->nextAck = 0;  session->previousAck = -1;
                session->nextAck1 = 0; session->previousAck1 = -1;
                session->nextAck2 = 0; session->previousAck2 = -1;
                session->nextAck4 = 0; session->previousAck4 = -1;
                session->nextAck5 = 0; session->previousAck5 = -1;

                // Reset output streams
                session->outputStream.sequence = 0;  session->outputStream.previousAck = -1;
                session->outputStream1.sequence = 0; session->outputStream1.previousAck = -1;
                session->outputStream2.sequence = 0; session->outputStream2.previousAck = -1;
                session->outputStream4.sequence = 0; session->outputStream4.previousAck = -1;
                session->outputStream5.sequence = 0; session->outputStream5.previousAck = -1;

                // Reset input streams
                session->inputStream.nextSequence = 0;  session->inputStream.previousAck = -1;
                session->inputStream.nextFragment = 0;  session->inputStream.previousProcessedFragment = -1;
                session->inputStream1.nextSequence = 0; session->inputStream1.previousAck = -1;
                session->inputStream1.nextFragment = 0; session->inputStream1.previousProcessedFragment = -1;
                session->inputStream2.nextSequence = 0; session->inputStream2.previousAck = -1;
                session->inputStream2.nextFragment = 0; session->inputStream2.previousProcessedFragment = -1;
                session->inputStream4.nextSequence = 0; session->inputStream4.previousAck = -1;
                session->inputStream4.nextFragment = 0; session->inputStream4.previousProcessedFragment = -1;
                session->inputStream5.nextSequence = 0; session->inputStream5.previousAck = -1;
                session->inputStream5.nextFragment = 0; session->inputStream5.previousProcessedFragment = -1;

                // Reset RC4 — must re-initialize key schedule, not just zero memory
                crypt_rc4_initialize(&session->inputStream.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->outputStream.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->inputStream1.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->inputStream2.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->inputStream4.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->inputStream5.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->outputStream1.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->outputStream2.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->outputStream4.rc4, app->rc4Decoded, app->rc4DecodedLen);
                crypt_rc4_initialize(&session->outputStream5.rc4, app->rc4Decoded, app->rc4DecodedLen);

                // Reset encryption flags (gateway LoginRequest will re-enable)
                session->inputStream.useEncryption = FALSE;
                session->outputStream.useEncryption = FALSE;
                session->inputStream1.useEncryption = FALSE;
                session->inputStream2.useEncryption = FALSE;
                session->inputStream4.useEncryption = FALSE;
                session->inputStream5.useEncryption = FALSE;

                // Reset fragment pools
                memset(&session->inputPool, 0, sizeof(FragmentPool));
                memset(&session->outputPool, 0, sizeof(FragmentPool));
                memset(&session->inputPool1, 0, sizeof(FragmentPool));
                memset(&session->outputPool1, 0, sizeof(FragmentPool));
                memset(&session->inputPool2, 0, sizeof(FragmentPool));
                memset(&session->outputPool2, 0, sizeof(FragmentPool));
                memset(&session->inputPool4, 0, sizeof(FragmentPool));
                memset(&session->outputPool4, 0, sizeof(FragmentPool));
                memset(&session->inputPool5, 0, sizeof(FragmentPool));
                memset(&session->outputPool5, 0, sizeof(FragmentPool));

                // Reset zone state
                session->needsProximityComplete = 0;
                session->characterReleased = FALSE;
                session->isReady = FALSE;
            }

            printf("\n");
            printf(MESSAGE_CONCAT_WARN("Ignoring connection args requested by client\n"));

            session->id = packet.sessionId;

            SessionReply sessionReply = {
                .sessionId = packet.sessionId,
                .crcSeed = session->args.crcSeed,
                .crcLen = session->args.crcLen,
                .compression = session->args.compression,
                .encryption = session->args.encryption,
                .udpLen = session->args.udpLen,
                .soeProtocolVersion = 3,
            };

            if (strcmp(packet.protocolName, "LoginUdp_11") == 0) {
                printf("[*] Enabling output encryption for session (input enabled after GatewayLoginRequest)\n");
                session->outputStream.useEncryption = TRUE;
            }

            CorePacketSend(app->socket, api, session->address.ip, session->address.port, &session->args,
                        CoreKindSessionReply, &sessionReply);
        } break;
        case CoreDisconnectId: {
            kind = CoreKindDisconnect;
            printf(MESSAGE_CONCAT_INFO("Received disconnect from client\n"));
        } break;
        case CorePingId: {
            kind = CoreKindPing;
            printf(MESSAGE_CONCAT_INFO("Received ping from client\n"));
        } break;
        case CoreMultiPacketId: {
            kind = CoreKindMultiPacket;
            printf(MESSAGE_CONCAT_INFO("Handling %s...\n"), coreKindNames[kind]);

            offset = CorePacketIdLen;

            if (session->args.compression) {
                offset++;
            }

            while (offset < dataLen - session->args.crcLen) {
                u32 chunkLen;
                offset += InputStreamReadLen((u8*)((uptr)data + offset), &chunkLen);

                CorePacketHandle(app, session, api, data + offset, chunkLen, TRUE);
                offset += chunkLen;
            }
        } break;
        case CoreDataId: {
            kind = CoreKindData;
            printf(MESSAGE_CONCAT_INFO("Handling %s...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream, packet.data, packet.dataLen,
                             packet.sequence, FALSE);
        } break;
        case CoreDataFragmentId: {
            kind = CoreKindDataFragment;
            printf(MESSAGE_CONCAT_INFO("Handling %s...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream, packet.data, packet.dataLen,
                             packet.sequence, TRUE);
        } break;
        case CoreAckId: {
            kind = CoreKindAck;
            printf(MESSAGE_CONCAT_INFO("Handling %s...\n"), coreKindNames[kind]);

            Ack packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            OutputStreamUpdateAck(&session->outputStream, (i32)packet.sequence);
        } break;

        // Channel 1
        case CoreData1Id: {
            kind = CoreKindData;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch1)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream1, packet.data, packet.dataLen,
                             packet.sequence, FALSE);
        } break;
        case CoreDataFragment1Id: {
            kind = CoreKindDataFragment;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch1)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream1, packet.data, packet.dataLen,
                             packet.sequence, TRUE);
        } break;
        case CoreAck1Id: {
            kind = CoreKindAck;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch1)...\n"), coreKindNames[kind]);

            Ack packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            OutputStreamUpdateAck(&session->outputStream1, (i32)packet.sequence);
        } break;

        // Channel 2
        case CoreData2Id: {
            kind = CoreKindData;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch2)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream2, packet.data, packet.dataLen,
                             packet.sequence, FALSE);
        } break;
        case CoreDataFragment2Id: {
            kind = CoreKindDataFragment;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch2)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream2, packet.data, packet.dataLen,
                             packet.sequence, TRUE);
        } break;
        case CoreAck2Id: {
            kind = CoreKindAck;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch2)...\n"), coreKindNames[kind]);

            Ack packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            OutputStreamUpdateAck(&session->outputStream2, (i32)packet.sequence);
        } break;

        // Channel 4
        case CoreData4Id: {
            kind = CoreKindData;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch4)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream4, packet.data, packet.dataLen,
                             packet.sequence, FALSE);
        } break;
        case CoreDataFragment4Id: {
            kind = CoreKindDataFragment;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch4)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream4, packet.data, packet.dataLen,
                             packet.sequence, TRUE);
        } break;
        case CoreAck4Id: {
            kind = CoreKindAck;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch4)...\n"), coreKindNames[kind]);

            Ack packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            OutputStreamUpdateAck(&session->outputStream4, (i32)packet.sequence);
        } break;

        // Channel 5
        case CoreData5Id: {
            kind = CoreKindData;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch5)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream5, packet.data, packet.dataLen,
                             packet.sequence, FALSE);
        } break;
        case CoreDataFragment5Id: {
            kind = CoreKindDataFragment;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch5)...\n"), coreKindNames[kind]);

            Data packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            InputStreamWrite(app, session, &session->inputStream5, packet.data, packet.dataLen,
                             packet.sequence, TRUE);
        } break;
        case CoreAck5Id: {
            kind = CoreKindAck;
            printf(MESSAGE_CONCAT_INFO("Handling %s (ch5)...\n"), coreKindNames[kind]);

            Ack packet = { 0 };
            CorePacketUnpack(data, dataLen, kind, &packet, isSubPacket, &session->args);

            OutputStreamUpdateAck(&session->outputStream5, (i32)packet.sequence);
        } break;
        default: {
            printf(MESSAGE_CONCAT_WARN("Unhandled core packet 0x%04x\n"), packetId);
            printf("[CORE DUMP] %u bytes: ", dataLen);
            for (u32 i = 0; i < (dataLen < 16 ? dataLen : 16); i++) {
                printf("%02x ", data[i]);
            }
            printf("\n");
            kind = CoreKindUnhandled;
            printf(MESSAGE_CONCAT_WARN("Unhandled core packet 0x%02x\n"), packetId);
        }
    }
}