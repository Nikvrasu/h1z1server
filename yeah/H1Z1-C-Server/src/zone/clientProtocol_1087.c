void ZonePacketSend(AppState* app, SessionState* session, Arena* arena, Zone_Packet_Kind kind,
                    void* packetPtr) {
    u8* baseBuffer = arena_push_size(arena, MAX_PACKET_LENGTH);
    u8* packedBuffer = baseBuffer + TunnelDataHeaderLen;

    u32 packedLen = zone_packet_pack(kind, packetPtr, packedBuffer);
    printf("[ZONE SEND] kind=%d packedLen=%u\n", kind, packedLen); 
    u32 totalLen = packedLen + TunnelDataHeaderLen;

    // arena_rewind(arena, maxLen - totalLen);
    GatewayTunnelDataSend(app, session, baseBuffer, totalLen);
}

void ZonePacketRawFileSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen, char* path) {
    u8* baseBuffer = arena_push_size(arena, maxLen);
    u8* packedBuffer = baseBuffer + TunnelDataHeaderLen;

    u32 packedLen = app->api->buffer_load_from_file(path, baseBuffer + TunnelDataHeaderLen, maxLen);
    u32 totalLen = packedLen + TunnelDataHeaderLen;

    arena_rewind(arena, maxLen - totalLen);
    GatewayTunnelDataSend(app, session, baseBuffer, totalLen);
}

void ZonePacketQueueLargeFile(AppState* app, SessionState* session, char* path, u32 fragsPerTick) {
    if (session->largeSendActive) {
        printf("[LARGE SEND] Already active, ignoring %s\n", path);
        return;
    }

    // Open and measure file
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("[LARGE SEND] Failed to open %s\n", path);
        return;
    }
    fseek(f, 0, SEEK_END);
    u32 fileLen = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate persistent buffer: 4-byte big-endian total length prefix + file data
    // This matches exactly what OutputStreamWrite writes for fragmented packets
    u32 bufLen = fileLen + 4;
    u8* buf = (u8*)malloc(bufLen);
    if (!buf) {
        printf("[LARGE SEND] malloc failed for %u bytes\n", bufLen);
        fclose(f);
        return;
    }

    // Write 4-byte big-endian total length prefix (OutputStreamWrite does this internally)
    endian_write_u32_big(buf, fileLen);
    fread(buf + 4, 1, fileLen, f);
    fclose(f);

    // Encrypt the entire buffer now (RC4 is stateful — must encrypt in order)
    if (session->outputStream.useEncryption) {
        crypt_rc4_transform(&session->outputStream.rc4, buf, bufLen);
    }

    session->largeSendBuffer = buf;
    session->largeSendTotalLen = bufLen;
    session->largeSendOffset = 0;
    session->largeSendFragsPerTick = fragsPerTick;
    session->largeSendActive = TRUE;

    printf("[LARGE SEND] Queued '%s' (%u bytes, ~%u ticks at %u frags/tick)\n",
           path, fileLen, (bufLen / 508) / fragsPerTick + 1, fragsPerTick);
}
void ZonePacketDrainLargeSend(AppState* app, SessionState* session) {
    if (!session->largeSendActive) return;

    u32 packetLen = 508; // MAX_PACKET_LENGTH - DATA_HEADER_LENGTH
    u32 sent = 0;

    while (sent < session->largeSendFragsPerTick && session->largeSendOffset < session->largeSendTotalLen) {
        u32 remaining = session->largeSendTotalLen - session->largeSendOffset;
        u32 chunkLen = MIN(remaining, packetLen);

        // Send directly via dataCallbackPtr — already encrypted, bypass OutputStreamWrite
        session->outputStream.sequence++;
        if (*session->outputStream.dataCallbackPtr) {
            (*session->outputStream.dataCallbackPtr)(
                app, session,
                session->largeSendBuffer + session->largeSendOffset,
                chunkLen,
                session->outputStream.sequence,
                TRUE  // isFragment=TRUE
            );
        }

        session->largeSendOffset += chunkLen;
        sent++;
    }

    if (session->largeSendOffset >= session->largeSendTotalLen) {
        free(session->largeSendBuffer);
        session->largeSendBuffer = NULL;
        session->largeSendActive = FALSE;
        printf("[LARGE SEND] Complete\n");
    }
}

void readPositionUpdateData(AppState* app, SessionState* session, u8* data, u32 offset) {
    Zone_Packet_PlayerUpdatePosition obj = { 0 }; // init
    u32 start_offset = offset; // offset is u32 for 2bit values, other than that, will need to typecast
                               // to a (u8 *) - doggo

    obj.flag = endian_read_u16_little(data + offset);
    offset += 2;

    obj.sequence = endian_read_u32_little(data + offset);
    offset += 4;

    obj.ubyte = endian_read_u8_little(data + offset);
    offset += 1;

    int2b v;           // signed, also maybe init this aswell?
    euler_angle euler; // needed for the last part of this packet

    if (obj.flag & 1) {
        v = endian_read_int2b_little(data, offset);
        session->stance = v.value;
        offset += v.length;
    }

    if (obj.flag & 2) {
        v = endian_read_int2b_little(data, offset);
        session->position.x = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        session->position.y = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        session->position.z = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 0x20) {
        session->orientation = endian_read_f32_little(data + offset);
        offset += 4;
    }

    if (obj.flag & 0x40) {
        v = endian_read_int2b_little(data, offset);
        session->frontTilt = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 0x80) {
        v = endian_read_int2b_little(data, offset);
        session->sideTilt = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 4) {
        v = endian_read_int2b_little(data, offset);
        session->angleChange = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 0x8) {
        v = endian_read_int2b_little(data, offset);
        session->verticalSpeed = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 0x10) {
        v = endian_read_int2b_little(data, offset);
        session->horizontalSpeed = v.value / 10;
        offset += v.length;
    }

    if (obj.flag & 0x100) {
        session->unkVec4;
        v = endian_read_int2b_little(data, offset);
        session->unkVec4.x = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        session->unkVec4.y = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        session->unkVec4.z = v.value / 100;
        offset += v.length;
    }

    if (obj.flag & 0x200) {
        v = endian_read_int2b_little(data, offset);
        euler.qx = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        euler.qy = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        euler.qz = v.value / 100;
        offset += v.length;
        v = endian_read_int2b_little(data, offset);
        euler.qw = v.value / 100;
        session->rotation = eul2quat(&euler);
        session->rotationRaw = euler;
        session->lookAt = eul2quat(&euler); // praying to the machine gods this works
        offset += v.length;
    }

    if (obj.flag & 0x400) {
        v = endian_read_int2b_little(data, offset);
        session->direction = v.value / 10;
        offset += v.length;
    }

    if (obj.flag & 0x800) {
        v = endian_read_int2b_little(data, offset);
        session->engineRpm = v.value / 10;
        offset += v.length;
    }

    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_PlayerUpdatePosition, &obj);
}

b32 IsValid(BaseItem* baseItem, u8 flag) {
    if (flag) {
        baseItem->debugFlag = flag;
    }

    if (baseItem->stackCount <= 0) {
        fprintf(stderr, "Item is invalid; itemDefId: (%u), stackCount: (%u), debugFlag: (%u)\n",
                baseItem->itemDefId, baseItem->stackCount, baseItem->debugFlag);
        return FALSE;
    }

    return TRUE;
}

ItemDefinition GetItemDefinition(ItemDefinition* itemDef, u32 itemDefId) {
    return itemDef[itemDefId];
}

b32 IsWeapon(ItemDefinition* itemDef, u32 itemDefId) {
    if (itemDefId != 0) {
        return itemDef[itemDefId].itemType == 20;
    }

    return FALSE;
}

b32 IsArmor(ItemDefinition* itemDef, u32 itemDefId) {
    if (itemDefId != 0) {
        if (itemDef[itemDefId].descriptionId == 12073 || itemDef[itemDefId].descriptionId == 11151
            || itemDef[itemDefId].descriptionId == 11153) {
            return TRUE;
        }
    }

    return FALSE;
}

b32 IsHelmet(ItemDefinition* itemDef, u32 itemDefId) {
    if (itemDefId != 0) {
        GetItemDefinition(itemDef, itemDefId);

        if (itemDef[itemDefId].descriptionId == 9945 || itemDef[itemDefId].descriptionId == 12994
            || itemDef[itemDefId].descriptionId == 9114 || itemDef[itemDefId].descriptionId == 9945) {
            return TRUE;
        }
    }

    return FALSE;
}

ItemData GetItemData(ItemData* itemData, ItemDefinition* itemDef, u32 itemDefId) {
    u32 durability = 0;

    while (TRUE) {
        if (IsWeapon(itemDef, itemDef->baseItem[itemDefId].itemDefId)) {
            durability = 2000;
            break;
        } else if (IsArmor(itemDef, itemDef->baseItem[itemDefId].itemDefId)) {
            durability = 1000;
            break;
        } else if (IsHelmet(itemDef, itemDef->baseItem[itemDefId].itemDefId)) {
            durability = 100;
            break;
        }
    }

    itemData->itemDefinitionId = itemDef->baseItem[itemDefId].itemDefId;
    itemData->tintId = 0;
    itemData->guid = itemDef->baseItem[itemDefId].itemGuid;
    itemData->count = itemDef->baseItem[itemDefId].stackCount;

    itemData->itemSubData->hasSubData = FALSE;

    itemData->containerGuid = itemDef->baseItem[itemDefId].containerGuid;
    itemData->containerDefinitionId = itemDef->baseItem[itemDefId].containerDefId;
    itemData->containerSlotId = itemDef->baseItem[itemDefId].slotId;
    itemData->baseDurability = durability;
    itemData->currentDurability = durability ? itemDef->baseItem[itemDefId].currentDurability : 0;
    itemData->maxDurabilityFromDefinition = durability;
    itemData->unknownBoolean1 = TRUE;
    itemData->ownerCharacterId = 0x0000000000000001ull;
    itemData->unknownDword9 = 1;
    /* Initialize weaponData here */

    return *itemData;
}

void WallOfDataBase(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    u8 subPacketId = data[2];
    i32 offset = sizeof(u8);

    switch (subPacketId) {
        case 5: {
            Zone_Packet_WallOfData_UIEvent uiEvent = { 0 };
            zone_packet_unpack(data + offset, dataLen - offset, Zone_Packet_Kind_WallOfData_UIEvent,
                               &uiEvent, &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_WallOfData_UIEvent, 0);
        } break;
        case 6: {
            Zone_Packet_WallOfData_ClientSystemInfo systemInfo = { 0 };
            zone_packet_unpack(data + offset, dataLen - offset,
                               Zone_Packet_Kind_WallOfData_ClientSystemInfo, &systemInfo,
                               &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_WallOfData_ClientSystemInfo, &systemInfo);
        } break;
        case 0xc: {
            Zone_Packet_WallOfData_ClientTransition clientTransition = { 0 };
            zone_packet_unpack(data + offset, dataLen - offset,
                               Zone_Packet_Kind_WallOfData_ClientTransition, &clientTransition,
                               &app->arenaPerTick);

            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_WallOfData_ClientTransition, &clientTransition);
        } break;
    }
}

void LobbyGameDefinitionsBase(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    u8 subPacketId = data[2];

    switch (subPacketId) {
        case 1: {
            ZonePacketSend(app, session, &app->arenaPerTick,
                           Zone_Packet_Kind_LobbyGameDefinition_DefinitionsResponse, 0);
        } break;
        default: {
        }
    }
}

void StaticViewBase(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Zone_Packet_StaticViewRequest request = { 0 };
    zone_packet_unpack(data + 3, dataLen - 3, Zone_Packet_Kind_StaticViewRequest, &request,
                       &app->arenaPerTick);

    printf("StaticView Loc: %s\n", request.viewpoint);

    if (strcmp(request.viewpoint.data, "kotkdefault") == 0) {
        Zone_Packet_ClientUpdate_UpdateLocation updateLoc = {
            .position = { .x = -32.26f, .y = 506.41f, .z = 280.21f, .w = 1.f },
            .rotation = { .x = -0.11f, .y = -0.58f, .z = -0.08f, .w = 1.f },
            .trigger_loading_screen = TRUE,
            .unk_u8_1 = 0,
            .unk_bool = FALSE,
        };
        ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientUpdate_UpdateLocation,
                       &updateLoc);

        Zone_Packet_StaticViewReply reply = {
            .state = 0,
            .position = { .x = 74.8f, .y = 201.5f, .z = 458.1f, .w = 99.01f },
            .rotation = { .x = 199.99f, .y = 289.99999f, .z = 370.17f, .w = 6.79f },
            .lookAt = { .x = 69.81f, .y = 56.f, .z = 0.f, .w = 0.f },
            .unk_byte_1 = 255,
            .enabled = TRUE,
        };
        ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_StaticViewReply, &reply);
    }
}
