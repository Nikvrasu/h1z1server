// ============================================================================
// Hex dump helper
// ============================================================================
void HexDumpBuffer(const char* label, u8* data, u32 len) {
    printf("\n[HEX DUMP] %s (%u bytes):\n", label, len);
    for (u32 i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");
}

void ZonePacketSendDebug(AppState* app, SessionState* session, Arena* arena, Zone_Packet_Kind kind,
                         void* packetPtr, const char* label) {
    u8* baseBuffer = arena_push_size(arena, MAX_PACKET_LENGTH);
    u8* packedBuffer = baseBuffer + TunnelDataHeaderLen;
    u32 packedLen = zone_packet_pack(kind, packetPtr, packedBuffer);
    printf("[ZONE SEND DEBUG] %s kind=%d packedLen=%u\n", label, kind, packedLen);
    HexDumpBuffer(label, packedBuffer, packedLen);
    u32 totalLen = packedLen + TunnelDataHeaderLen;
    GatewayTunnelDataSend(app, session, baseBuffer, totalLen);
}

static u32 TraceCrc32(const u8* data, u32 len) {
    u32 crc = 0xFFFFFFFFu;
    for (u32 i = 0; i < len; i++) {
        crc ^= data[i];
        for (u32 b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(i32)(crc & 1u)));
        }
    }
    return ~crc;
}

static const u8 kLtfpcTemplate[] = {
    0xda, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69,
    0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x48, 0x65, 0x61, 0x64, 0x5f, 0x30, 0x31, 0x2e,
    0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66, 0x61, 0x75,
    0x6c, 0x74, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1a, 0x00, 0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65,
    0x5f, 0x43, 0x68, 0x65, 0x73, 0x74, 0x5f, 0x42, 0x72, 0x61, 0x2e, 0x61, 0x64, 0x72, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x01, 0x00, 0x00,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x53,
    0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x4c, 0x65, 0x67, 0x73,
    0x5f, 0x50, 0x61, 0x6e, 0x74, 0x73, 0x5f, 0x55, 0x6e, 0x64, 0x65, 0x72, 0x77, 0x65, 0x61, 0x72,
    0x2e, 0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66, 0x61,
    0x75, 0x6c, 0x74, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x57, 0x65, 0x61, 0x70, 0x6f, 0x6e, 0x5f, 0x45, 0x6d, 0x70, 0x74,
    0x79, 0x2e, 0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66,
    0x61, 0x75, 0x6c, 0x74, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61,
    0x6c, 0x65, 0x5f, 0x45, 0x79, 0x65, 0x73, 0x5f, 0x30, 0x31, 0x2e, 0x61, 0x64, 0x72, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x01, 0x00, 0x00,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x53,
    0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x43, 0x68, 0x65, 0x73,
    0x74, 0x5f, 0x48, 0x6f, 0x6f, 0x64, 0x69, 0x65, 0x5f, 0x44, 0x6f, 0x77, 0x6e, 0x2e, 0x61, 0x64,
    0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74,
    0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00,
    0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x4c,
    0x65, 0x67, 0x73, 0x5f, 0x50, 0x61, 0x6e, 0x74, 0x73, 0x5f, 0x53, 0x6b, 0x69, 0x6e, 0x6e, 0x79,
    0x4c, 0x65, 0x67, 0x2e, 0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44,
    0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72,
    0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x46, 0x65, 0x65, 0x74, 0x5f, 0x43, 0x6f, 0x6e, 0x76, 0x65, 0x79,
    0x73, 0x2e, 0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x44, 0x65, 0x66,
    0x61, 0x75, 0x6c, 0x74, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61,
    0x6c, 0x65, 0x5f, 0x48, 0x65, 0x61, 0x64, 0x5f, 0x30, 0x31, 0x2e, 0x61, 0x64, 0x72, 0x21, 0x00,
    0x00, 0x00, 0x53, 0x75, 0x72, 0x76, 0x69, 0x76, 0x6f, 0x72, 0x4d, 0x61, 0x6c, 0x65, 0x5f, 0x48,
    0x61, 0x69, 0x72, 0x5f, 0x4d, 0x65, 0x64, 0x69, 0x75, 0x6d, 0x4d, 0x65, 0x73, 0x73, 0x79, 0x2e,
    0x61, 0x64, 0x72, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x10,
    0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06,
    0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xae, 0xa7, 0x94, 0xc3, 0xae, 0x07, 0xfd, 0x43, 0xcd, 0xf0, 0x98, 0xc5,
};

static const u32 kLtfpcTemplateLen = sizeof(kLtfpcTemplate);

static void TraceLightweightToFullPcPayloadBytes(SessionState* session, const u8* payload,
                                                 u32 payloadLen, const char* source);

static i32 FindBytes(const u8* haystack, u32 haystackLen, const u8* needle, u32 needleLen) {
    if (!haystack || !needle || needleLen == 0 || haystackLen < needleLen) {
        return -1;
    }
    for (u32 i = 0; i + needleLen <= haystackLen; i++) {
        if (memcmp(haystack + i, needle, needleLen) == 0) {
            return (i32)i;
        }
    }
    return -1;
}

static b8 PatchTemplateStringInPlace(u8* buffer, u32 bufferLen, const char* fromLiteral,
                                     String8 toValue) {
    u32 fromLen = (u32)strlen(fromLiteral);
    i32 idx = FindBytes(buffer, bufferLen, (const u8*)fromLiteral, fromLen);
    if (idx < 0) {
        return FALSE;
    }

    // Keep template layout stable: truncate to original slot length if needed.
    u32 copyLen = toValue.size < fromLen ? toValue.size : fromLen;
    memset(buffer + idx, 0, fromLen);
    memcpy(buffer + idx, toValue.data, copyLen);

    // Most strings in this payload are prefixed by a u32 length right before the bytes.
    if ((u32)idx >= 4) {
        u32 oldLen = endian_read_u32_little(buffer + idx - 4);
        if (oldLen == fromLen) {
            endian_write_u32_little(buffer + idx - 4, copyLen);
        }
    }

    return TRUE;
}

static void SendLightweightToFullPcDynamic(AppState* app, SessionState* session, Arena* arena) {
    u8* payload = arena_push_size(arena, KB(4));
    u32 offset = 0;

    u32 gender = session->pGetPlayerActor.gender ? session->pGetPlayerActor.gender : 1;
    String8 headActor = session->pGetPlayerActor.headActor;
    if (headActor.size == 0) {
        headActor = gender == 2 ? STR8("SurvivorFemale_Head_01.adr") : STR8("SurvivorMale_Head_01.adr");
    }
    String8 hairModel = session->pGetPlayerActor.hairModel;
    if (hairModel.size == 0) {
        hairModel = gender == 2 ? STR8("SurvivorFemale_Hair_Down.adr") : STR8("SurvivorMale_Hair_MediumMessy.adr");
    }

    String8 chestModel = gender == 2 ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 legsModel = gender == 2 ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
    String8 eyesModel = gender == 2 ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");

    String8 attachmentModels[8] = {
        headActor,
        chestModel,
        legsModel,
        STR8("Weapon_Empty.adr"),
        eyesModel,
        STR8("SurvivorMale_Chest_Hoodie_Down.adr"),
        STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),
        STR8("SurvivorMale_Feet_Conveys.adr"),
    };
    u32 attachmentSlots[8] = { 1, 3, 4, 7, 105, 10, 14, 13 };

    // Explicitly serialize 0xDA attachment-group header and entries.
    endian_write_u8_little(payload + offset, 0xda);
    offset += sizeof(u8);
    endian_write_u8_little(payload + offset, 0x00);
    offset += sizeof(u8);
    endian_write_u8_little(payload + offset, 0x04);
    offset += sizeof(u8);
    endian_write_u32_little(payload + offset, ARRAY_COUNT(attachmentModels));
    offset += sizeof(u32);

    for (u32 i = 0; i < ARRAY_COUNT(attachmentModels); i++) {
        String8 model = attachmentModels[i];
        endian_write_u32_little(payload + offset, model.size);
        offset += sizeof(u32);
        memcpy(payload + offset, model.data, model.size);
        offset += model.size;

        endian_write_u32_little(payload + offset, 0);  // unk
        offset += sizeof(u32);

        endian_write_u32_little(payload + offset, 7);  // tint alias len
        offset += sizeof(u32);
        memcpy(payload + offset, "Default", 7);
        offset += 7;

        endian_write_u32_little(payload + offset, 1);  // decal alias len
        offset += sizeof(u32);
        memcpy(payload + offset, "#", 1);
        offset += 1;

        endian_write_u32_little(payload + offset, 0);
        offset += sizeof(u32);
        endian_write_u32_little(payload + offset, 0);
        offset += sizeof(u32);
        endian_write_u32_little(payload + offset, 0);
        offset += sizeof(u32);

        endian_write_u32_little(payload + offset, attachmentSlots[i]);
        offset += sizeof(u32);

        endian_write_u32_little(payload + offset, 0);
        offset += sizeof(u32);
        endian_write_u32_little(payload + offset, 0);
        offset += sizeof(u32);
    }

    i32 headBangIdx = FindBytes(kLtfpcTemplate, kLtfpcTemplateLen,
                                (const u8*)"SurvivorMale_Head_01.adr!",
                                (u32)strlen("SurvivorMale_Head_01.adr!"));
    if (headBangIdx < 4) {
        printf("[LTFPC DYN] failed to locate template tail anchor, aborting send\n");
        return;
    }

    u32 tailOffset = (u32)(headBangIdx - 4);
    u32 tailLen = kLtfpcTemplateLen - tailOffset;
    memcpy(payload + offset, kLtfpcTemplate + tailOffset, tailLen);
    offset += tailLen;

    u32 payloadLen = offset;

    struct ltfpc_patch_s {
        const char* from;
        String8 to;
    } patchList[] = {
        { "SurvivorMale_Hair_MediumMessy.adr", hairModel },
    };

    u32 patchedCount = 0;
    for (u32 i = 0; i < ARRAY_COUNT(patchList); i++) {
        patchedCount += PatchTemplateStringInPlace(payload, payloadLen, patchList[i].from,
                                                   patchList[i].to)
                            ? 1
                            : 0;
    }

    // One template variant contains this head string with trailing punctuation.
    String8 headActorBang = headActor;
    patchedCount += PatchTemplateStringInPlace(payload, payloadLen, "SurvivorMale_Head_01.adr!",
                                               headActorBang)
                        ? 1
                        : 0;

    u32 dynCrc = TraceCrc32(payload, payloadLen);
    printf("[LTFPC DYN] source=explicit-attachments len=%u crc32=0x%08x patched=%u gender=%u head='%.*s' hair='%.*s'\n",
           payloadLen,
           dynCrc,
           patchedCount,
           gender,
           (int)headActor.size, headActor.data,
           (int)hairModel.size, hairModel.data);
    TraceLightweightToFullPcPayloadBytes(session, payload, payloadLen, "dynamic-built");

    u8* baseBuffer = arena_push_size(arena, payloadLen + TunnelDataHeaderLen);
    memcpy(baseBuffer + TunnelDataHeaderLen, payload, payloadLen);
    GatewayTunnelDataSend(app, session, baseBuffer, payloadLen + TunnelDataHeaderLen);
}

static void TraceLifecycleState(const char* tag, SessionState* session) {
    __time64_t now;
    _time64(&now);
    printf("[TRACE LIFE] %s ts=%lld guid=0x%llx char=0x%llx transient=%u cycle=%u deployedCycle=%u isReady=%d finished=%d released=%d deployed=%d\n",
           tag,
           now,
           (unsigned long long)session->guid,
           (unsigned long long)session->characterId,
           session->transientId,
           session->zoneCycleId,
           session->deployedCycleId,
           session->isReady,
           session->finished_loading,
           session->characterReleased,
           session->characterDeployed);
}

static void TraceLightweightToFullPcPayloadBytes(SessionState* session, const u8* payload,
                                                 u32 payloadLen, const char* source) {
    if (!payload || payloadLen == 0) {
        printf("[TRACE LTFPC] source='%s' empty payload\n", source);
        return;
    }

    u32 crc32 = TraceCrc32(payload, payloadLen);
    printf("[TRACE LTFPC] source='%s' fileLen=%u crc32=0x%08x guid=0x%llx char=0x%llx transient=%u cycle=%u\n",
           source,
           payloadLen,
           crc32,
           (unsigned long long)session->guid,
           (unsigned long long)session->characterId,
           session->transientId,
           session->zoneCycleId);

    u32 headLen = payloadLen < 16 ? payloadLen : 16;
    u32 tailLen = payloadLen < 16 ? payloadLen : 16;

    printf("[TRACE LTFPC] head=");
    for (u32 i = 0; i < headLen; i++) {
        printf("%02x", payload[i]);
        if (i + 1 < headLen) printf(" ");
    }
    printf("\n");

    printf("[TRACE LTFPC] tail=");
    for (u32 i = payloadLen - tailLen; i < payloadLen; i++) {
        printf("%02x", payload[i]);
        if (i + 1 < payloadLen) printf(" ");
    }
    printf("\n");
}


// ============================================================================
// SendEquipmentAndMovement — called from ClientFinishedLoading
// ============================================================================
void SendEquipmentAndMovement(AppState* app, SessionState* session) {
    __time64_t eqTime;
    _time64(&eqTime);
    static int eqCount = 0;
    eqCount++;
    PRINT_TIMESTAMP(); printf("========== SEND EQUIPMENT & MOVEMENT (ClientFinishedLoading) ==========\n");
    printf("[EQUIP] SendEquipmentAndMovement called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d characterDeployed=%d\n",
           eqCount, eqTime, (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased,
           session->characterDeployed);

    // 1. GameTimeSync
    Zone_Packet_GameTimeSync gameTimeSync = { 0 };
    gameTimeSync.cycle_speed = 0.0f;
    gameTimeSync.time = 300000;
    gameTimeSync.unk_bool = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_GameTimeSync, &gameTimeSync);

    // 2. UpdateWeatherData
    Zone_Packet_UpdateWeatherData updt_weather_data = {
        .overcast = 1.0f,
        .fogDensity = 0.000173f,
        .fogFloor = 10.0f,
        .fogGradient = 0.0144f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity = 0,
        .cloudWeight0 = 0.05f,
        .cloudWeight1 = 0.0f,
        .cloudWeight2 = 0.05f,
        .cloudWeight3 = 0.15f,
        .transitionTime = 0,
        .sunAxisX = 38,
        .sunAxisY = -15,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = -1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8("sky_Z_clouds.dds"),
        .stratusCloudTiling = 0.30f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight = 1000,
        .cumulusCloudTiling = 0.20f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness = 0.25f,
        .cloudSilverLiningBrightness = 7.0f,
        .cloudShadows = 0.5f,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_UpdateWeatherData,
                   &updt_weather_data);

    // 3. Character.WeaponStance
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 4. Equipment.SetCharacterEquipment — profile_id=3, unk_bool_2=FALSE (initial set not update)
    u32 gender = session->pGetPlayerActor.gender;
    if (gender == 0) gender = 1;

    String8 eqHeadActor = session->pGetPlayerActor.headActor;
    if (eqHeadActor.size == 0) eqHeadActor = STR8("SurvivorMale_Head_01.adr");

    String8 eqChestModel = (gender == 2) ? STR8("SurvivorFemale_Chest_Bra.adr") : STR8("SurvivorMale_Chest_Bra.adr");
    String8 eqLegsModel  = (gender == 2) ? STR8("SurvivorFemale_Legs_Pants_Underwear.adr") : STR8("SurvivorMale_Legs_Pants_Underwear.adr");
    String8 eqEyesModel  = (gender == 2) ? STR8("SurvivorFemale_Eyes_01.adr") : STR8("SurvivorMale_Eyes_01.adr");

    Zone_Packet_Equipment_SetCharacterEquipment setEquipment = { 0 };
    setEquipment.unk_string_1 = STR8("Default");
    setEquipment.unk_string_2 = STR8("#");
    setEquipment.unk_bool_2 = FALSE;
    setEquipment.length_1 = (struct length_1_s[1]){[0] = {
        .character_id = session->characterId,
        .profile_id = 4,
    }};
    setEquipment.equipment_slot_array_count = 8;
    setEquipment.equipment_slot_array = (struct equipment_slot_array_s[8]){
        [0] = { .equipment_slot_id_1 = 1,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 1,   .guid =      0,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [1] = { .equipment_slot_id_1 = 3,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 3,   .guid = 0x1001,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [2] = { .equipment_slot_id_1 = 4,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 4,   .guid = 0x1002,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [3] = { .equipment_slot_id_1 = 7,   .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 7,   .guid = ITEM_GUID_FISTS, .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [4] = { .equipment_slot_id_1 = 105, .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 105, .guid = 0x1004,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [5] = { .equipment_slot_id_1 = 10,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 10,  .guid = 0x1005,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [6] = { .equipment_slot_id_1 = 14,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 14,  .guid = 0x1006,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
        [7] = { .equipment_slot_id_1 = 13,  .length_2 = (struct length_2_s[1]){[0] = { .equipment_slot_id_2 = 13,  .guid = 0x1007,          .tint_alias = STR8("Default"), .decal_alias = STR8("#") }} },
    };
    setEquipment.attachments_data_1_count = 8;
    setEquipment.attachments_data_1 = (struct attachments_data_1_s[8]){
        [0] = { .model_name = eqHeadActor,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1   },
        [1] = { .model_name = eqChestModel,                                   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3   },
        [2] = { .model_name = eqLegsModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4   },
        [3] = { .model_name = STR8("Weapon_Empty.adr"),                       .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7   },
        [4] = { .model_name = eqEyesModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105 },
        [5] = { .model_name = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),     .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 10  },
        [6] = { .model_name = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),  .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 14  },
        [7] = { .model_name = STR8("SurvivorMale_Feet_Conveys.adr"),          .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 13  },
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Equipment_SetCharacterEquipment, &setEquipment);
    printf("[EQUIP] Sent Equipment.SetCharacterEquipment (8 slots, profile_id=3, unk_bool_2=FALSE)\n");

    // 5. ClientUpdate.ActivateProfile — profile_id=3, full attachment list, actor_model_id set
    Zone_Packet_ClientUpdate_ActivateProfile activateProfile = { 0 };
    activateProfile.profile_payload = (struct profile_payload_s[1]){
        [0] = {
            .profile_id   = 4,
            .name_id      = 0,
            .desc_id      = 0,
            .type         = 3,
            .unk_dword_1  = 0,
            .ability_bg_image_set = 0,
            .badge_image_set      = 0,
            .button_image_set     = 0,
            .unk_byte_1   = 0,
            .unk_byte_2   = 0,
            .unk_dword_2  = 0,
            .unk_list_1_count = 0,
            .unk_dword_6  = 0,
            .unk_dword_7  = 0,
            .unk_byte_3   = 0,
            .unk_float_1  = 1.7f,
            .unk_float_2  = 0.95f,
            .unk_float_3  = 0.0f,
            .unk_dword_8  = 0,
            .unk_float_4  = 0.0f,
            .unk_dword_9  = 0,
            .unk_dword_10 = 0,
            .unk_dword_11 = 0,
            .unk_dword_12 = 0,
            .unk_dword_13 = 0,
        },
    };
    activateProfile.attachment_list_count = 8;
    activateProfile.attachment_list = (struct attachment_list_s[8]){
        [0] = { .model_name = eqHeadActor,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 1,   .unk_bool_1 = FALSE },
        [1] = { .model_name = eqChestModel,                                   .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 3,   .unk_bool_1 = FALSE },
        [2] = { .model_name = eqLegsModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 4,   .unk_bool_1 = FALSE },
        [3] = { .model_name = STR8("Weapon_Empty.adr"),                       .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 7,   .unk_bool_1 = FALSE },
        [4] = { .model_name = eqEyesModel,                                    .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 105, .unk_bool_1 = FALSE },
        [5] = { .model_name = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),     .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 10,  .unk_bool_1 = FALSE },
        [6] = { .model_name = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),  .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 14,  .unk_bool_1 = FALSE },
        [7] = { .model_name = STR8("SurvivorMale_Feet_Conveys.adr"),          .tint_alias = STR8("Default"), .decal_alias = STR8("#"), .slot_id = 13,  .unk_bool_1 = FALSE },
    };
    activateProfile.unk_dword_1    = 0;
    activateProfile.unk_dword_2    = 0;
    activateProfile.actor_model_id = session->pGetPlayerActor.actorModelId;
    activateProfile.tint_alias     = STR8("Default");
    activateProfile.decal_alias    = STR8("#");
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ActivateProfile, &activateProfile);
    printf("[EQUIP] Sent ActivateProfile (profile_id=3, 8 attachments, actor_model_id=%u)\n",
           session->pGetPlayerActor.actorModelId);

    // 6. Loadout.SetLoadoutSlots
    Zone_Packet_Loadout_SetLoadoutSlots loadoutSlots = { 0 };
    loadoutSlots.character_id = session->characterId;
    loadoutSlots.loadout_id = LOADOUT_ID_KOTK_CHARACTER;
    loadoutSlots.loadout_slot_data_count = 5;
    loadoutSlots.loadout_slot_data = (struct loadout_slot_data_s[5]){
        [0] = {
            .hotbar_slot_id = LOADOUT_SLOT_MELEE,
            .loadout_id_1   = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id        = LOADOUT_SLOT_MELEE,
            .item_def_id1   = WEAPON_FISTS,
            .loadout_item_guid = ITEM_GUID_FISTS,
            .unk_byte_1     = 0,
            .unk_dword_1    = 0,
        },
        [1] = {
            .hotbar_slot_id = LOADOUT_SLOT_BINOCULARS,
            .loadout_id_1   = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id        = LOADOUT_SLOT_BINOCULARS,
            .item_def_id1   = WEAPON_BINOCULARS,
            .loadout_item_guid = ITEM_GUID_BINOCULARS,
            .unk_byte_1     = 0,
            .unk_dword_1    = 0,
        },
        [2] = {
            .hotbar_slot_id    = 10,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 10,
            .item_def_id1      = 5747,
            .loadout_item_guid = 0x1005,
        },
        [3] = {
            .hotbar_slot_id    = 14,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 14,
            .item_def_id1      = 2178,
            .loadout_item_guid = 0x1006,
        },
        [4] = {
            .hotbar_slot_id    = 13,
            .loadout_id_1      = LOADOUT_ID_KOTK_CHARACTER,
            .slot_id           = 13,
            .item_def_id1      = 2216,
            .loadout_item_guid = 0x1007,
        },
    };
    loadoutSlots.current_slot_id = LOADOUT_SLOT_MELEE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Loadout_SetLoadoutSlots, &loadoutSlots);

    // 7. Command.RunSpeed
    Zone_Packet_Command_RunSpeed runSpeed = { .run_speed = 7.5f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Command_RunSpeed, &runSpeed);

    // 8. ClientUpdate.ModifyMovementSpeed
    Zone_Packet_ClientUpdate_ModifyMovementSpeed moveSpeed = { 0 };
    moveSpeed.speed = 2.0f;
    moveSpeed.movementVersion = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_ModifyMovementSpeed, &moveSpeed);

    printf("========== SEND EQUIPMENT & MOVEMENT END ==========\n\n");
}


void DeployCharacter(AppState* app, SessionState* session) {
    __time64_t timer;
    __time64_t deployTime;
    _time64(&timer);
    _time64(&deployTime);
    static int deployCount = 0;
    deployCount++;
    printf("[DEPLOY] DeployCharacter called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx isReady=%d finished_loading=%d characterReleased=%d characterDeployed=%d zoneCycleId=%u deployedCycleId=%u\n",
           deployCount, deployTime, (unsigned long long)session->characterId,
           session->isReady, session->finished_loading, session->characterReleased,
           session->characterDeployed, session->zoneCycleId, session->deployedCycleId);

    // Ensure first login on older sessions gets a valid cycle.
    if (session->zoneCycleId == 0) {
        session->zoneCycleId = 1;
    }

    // Deploy exactly once per zone cycle; protects ClientIsReady/0x97 race.
    if (session->deployedCycleId == session->zoneCycleId) {
        printf("[DEPLOY GUARD] Duplicate deploy blocked for zoneCycleId=%u\n",
               session->zoneCycleId);
        return;
    }
    printf("[DEPLOY] Session actor: model=%u gender=%u head=%u hair='%.*s' headActor='%.*s'\n",
           session->pGetPlayerActor.actorModelId, session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType,
           (int)session->pGetPlayerActor.hairModel.size, session->pGetPlayerActor.hairModel.data,
           (int)session->pGetPlayerActor.headActor.size, session->pGetPlayerActor.headActor.data);

    PRINT_TIMESTAMP(); printf("========== DEPLOY CHARACTER BEGIN ==========\n");

    // 1. POIChangeMessage
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_POIChangeMessage, 0);

    // 2. Character.UpdateCharacterState — show fully visible
    Zone_Packet_Character_UpdateCharacterState showState = { 0 };
    showState.character_id = session->characterId;
    showState.state1 = 1;
    showState.state2 = 1;
    showState.state3 = 1;
    showState.state4 = 1;
    showState.state5 = 1;
    showState.state6 = 1;
    showState.state7 = 1;
    showState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &showState);
    printf("[STATE] Sent CharacterState: all visible (state1-7=1)\n");

    // 3. DoneSendingPreloadCharacters
    Zone_Packet_ClientUpdate_DoneSendingPreloadCharacters preloadDone = { 0 };
    preloadDone.is_done = TRUE;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_DoneSendingPreloadCharacters, &preloadDone);

    // UpdateCamera
    u8 updateCamera[] = { 0x57 };
    u8* camBuf = arena_push_size(&app->arenaPerTick, sizeof(updateCamera) + TunnelDataHeaderLen);
    memcpy(camBuf + TunnelDataHeaderLen, updateCamera, sizeof(updateCamera));
    GatewayTunnelDataSend(app, session, camBuf, sizeof(updateCamera) + TunnelDataHeaderLen);

    // 4. DtoObjectInitialData
    {
        u8 dtoData[] = {
            0x05, 0x03,
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(dtoData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, dtoData, sizeof(dtoData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(dtoData) + TunnelDataHeaderLen);
        printf("[DEPLOY] Sent DtoObjectInitialData (raw 0x0503)\n");
    }

    session->characterReleased = TRUE;
    session->characterDeployed = TRUE;
    session->deployedCycleId = session->zoneCycleId;

    // 5. Character.CharacterStateDelta
    Zone_Packet_Character_CharacterStateDelta stateDelta = { 0 };
    stateDelta.guid_1 = session->characterId;
    stateDelta.guid_3 = 0x40000000ull;
    stateDelta.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_CharacterStateDelta, &stateDelta);

    // 5b. AddLightweightPc — registers self as world entity, initializes CharacterAttachmentGroup
    printf("[DEPLOY] Sending AddLightweightPc...\n");
    Zone_Packet_AddLightweightPc lightweightPc = { 0 };
    lightweightPc.character_id       = session->characterId;
    lightweightPc.transient_id.value = session->transientId ? session->transientId : 1;
    lightweightPc.id_characterName   = session->characterName;
    lightweightPc.actorModelId       = session->pGetPlayerActor.actorModelId;
    lightweightPc.position           = (vec3){ -297.309998f, 506.059998f, -4894.100098f };
    lightweightPc.rotation           = (vec4){ 0.0f, -0.707100f, 0.0f, 0.707100f };
    lightweightPc.unknownFloat1      = 1.0f;
    lightweightPc.flags1             = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_AddLightweightPc, &lightweightPc);
    printf("[DEPLOY] Sent AddLightweightPc\n");

    // 6. Equipment + movement — prefer ClientFinishedLoading timing.
    // If the client finished loading early, send immediately from here.
    if (session->finished_loading) {
        printf("[DEPLOY] ClientFinishedLoading already set; sending equipment immediately\n");
        SendEquipmentAndMovement(app, session);
    } else {
        printf("[DEPLOY] Waiting for ClientFinishedLoading before equipment/attachments\n");
    }

    // 7. LightweightToFullPc — full character upgrade with position
    printf("[DEPLOY] Sending LightweightToFullPc...\n");
    static u32 ltfpcSendCount = 0;
    ltfpcSendCount += 1;
    printf("[TRACE LTFPC] sendCount=%u sendSeqDebug=%u\n", ltfpcSendCount, session->sendSeqDebug);
    TraceLifecycleState("before-LightweightToFullPc", session);
    SendLightweightToFullPcDynamic(app, session, &app->arenaPerTick);
    TraceLifecycleState("after-LightweightToFullPc", session);
    printf("[DEPLOY] LightweightToFullPc sent\n");

    // 8. ZoneDoneSendingInitialData
    {
        __time64_t zdSendTime;
        _time64(&zdSendTime);
        printf("[TIMING] ZoneDoneSendingInitialData sent at %lld\n", zdSendTime);
    }
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ZoneDoneSendingInitialData, 0);

    // 9. ResourceEventBase
    {
        Zone_Packet_ResourceEventBase resourceEvent = { 0 };
        resourceEvent.gametime = timer & 0x7fffffff;
        resourceEvent.variabletype8_case = 0;
        resourceEvent.variabletype8.set_character_resources_1.character_id_1 = session->characterId;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1_count = 4;
        resourceEvent.variabletype8.set_character_resources_1.character_resources_1 = (struct character_resources_1_s[4]){
            [0] = { .resource_type_1 = HEALTHTYPE,    .resource_id_1 = HEALTHID,    .resource_type_2 = HEALTHTYPE,    .value = 10000 },
            [1] = { .resource_type_1 = HUNGERTYPE,    .resource_id_1 = HUNGERID,    .resource_type_2 = HUNGERTYPE,    .value = 10000 },
            [2] = { .resource_type_1 = HYDRATIONTYPE, .resource_id_1 = HYDRATIONID, .resource_type_2 = HYDRATIONTYPE, .value = 10000 },
            [3] = { .resource_type_1 = STAMINATYPE,   .resource_id_1 = STAMINAID,   .resource_type_2 = STAMINATYPE,   .value = 10000 },
        };
        ZonePacketSend(app, session, &app->arenaPerTick,
                    Zone_Packet_Kind_ResourceEventBase, &resourceEvent);
    }

    // 10. AccountItemManagerStateChanged
    {
        u8 escrowData[] = {
            0x23, 0x00,
            0x00,
            0x01,
            0x01,
            0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(escrowData) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, escrowData, sizeof(escrowData));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(escrowData) + TunnelDataHeaderLen);
        printf("[DEPLOY] Sent AccountItemManagerStateChanged\n");
    }

    // 11. WeaponStance
    Zone_Packet_Character_WeaponStance weaponStance = { 0 };
    weaponStance.character_id = session->characterId;
    weaponStance.stance = 0;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_WeaponStance, &weaponStance);

    // 12. Deferred NetworkProximityUpdatesComplete
    session->needsProximityComplete = 1;
    _time64(&session->proximityCompleteTime);
    session->proximityCompleteTime += 5;

    printf("[TIMER_DEFER] NetworkProximityUpdatesComplete scheduled for +5s\n");
    printf("========== DEPLOY CHARACTER END ==========\n\n");
}


void OnLogin(AppState* app, SessionState* session) {
    __time64_t onLoginTime;
    _time64(&onLoginTime);
    __time64_t timer;
    _time64(&timer);
    static int onLoginCount = 0;
    onLoginCount++;
    printf("[ONLOGIN] OnLogin called %d time(s) total [TIMESTAMP=%lld] charId=0x%llx\n",
           onLoginCount, onLoginTime, (unsigned long long)session->characterId);

    // FIRST: ensure actor data has valid defaults before any packet uses it.
    // pGetPlayerActor is populated by the login server's GetHeadTypeId, but
    // the zone server session starts fresh. If the data didn't transfer, default to male head 1.
    if (session->pGetPlayerActor.actorModelId == 0) {
        session->pGetPlayerActor.actorModelId = 9469;
        session->pGetPlayerActor.gender       = 1;
        session->pGetPlayerActor.headType     = 1;
        session->pGetPlayerActor.headActor    = STR8("SurvivorMale_Head_01.adr");
        session->pGetPlayerActor.hairModel    = STR8("SurvivorMale_Hair_MediumMessy.adr");
        printf("[ONLOGIN] Actor data was empty — applied male head 1 defaults\n");
    }

    PRINT_TIMESTAMP(); printf("[*] [TIMING] OnLogin BEGIN: 0. Hide Character\n");

    // 0. Character.UpdateCharacterState — hide during loading (state1=0)
    Zone_Packet_Character_UpdateCharacterState hideState = { 0 };
    hideState.character_id = session->characterId;
    hideState.state1 = 0;
    hideState.game_time = timer & 0x7fffffff;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateCharacterState, &hideState);
    printf("[STATE] Sent CharacterState: hidden (state1=0)\n");

    PRINT_TIMESTAMP(); printf("[*] [TIMING] OnLogin BEGIN: 1. InitializationParameters\n");

    // 1. InitializationParameters
    Zone_Packet_InitializationParameters init_params = {
        .environment  = STR8("LIVE_KOTK"),
        .unk_string_1 = STR8("SKU_Is_KotK"),
        .ruleset_definitions_count = 0,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_InitializationParameters,
                   &init_params);

    PRINT_TIMESTAMP(); printf("[*] [TIMING] 2. SendZoneDetails\n");

    // 2. SendZoneDetails
    Zone_Packet_SendZoneDetails send_zone_details = {
        .zone_name = STR8("Z2"),
        .zone_type = 4,
        .unk_bool  = FALSE,
        .overcast  = 1.0f,
        .fogDensity = 0.000173f,
        .fogFloor   = 10.0f,
        .fogGradient = 0.0144f,
        .globalPrecipitation = 0,
        .temperature = 75,
        .skyClarity  = 0,
        .cloudWeight0 = 0.05f,
        .cloudWeight1 = 0.0f,
        .cloudWeight2 = 0.05f,
        .cloudWeight3 = 0.15f,
        .transitionTime = 0,
        .sunAxisX = 38,
        .sunAxisY = -15,
        .sunAxisZ = 0,
        .windDirX = -1.0f,
        .windDirY = -0.5f,
        .windDirZ = -1.0f,
        .wind = 3,
        .rainMinStrength = 0,
        .rainRampUpTimeSeconds = 1,
        .cloudFile = STR8("sky_Z_clouds.dds"),
        .stratusCloudTiling  = 0.30f,
        .stratusCloudScrollU = -0.002f,
        .stratusCloudScrollV = 0,
        .stratusCloudHeight  = 1000,
        .cumulusCloudTiling  = 0.20f,
        .cumulusCloudScrollU = 0,
        .cumulusCloudScrollV = 0.002f,
        .cumulusCloudHeight  = 8000,
        .cloudAnimationSpeed = 0,
        .cloudSilverLiningThickness  = 0.25f,
        .cloudSilverLiningBrightness = 7.0f,
        .cloudShadows = 0.5f,
        .zone_id   = 5,
        .zone_id_2 = 5,
        .name_id   = 61609,
        .unk_bool2 = TRUE,
        .lighting  = STR8("Lighting_Z2.txt"),
        .unk_bool3 = FALSE,
        .unk_bool4 = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendZoneDetails,
                   &send_zone_details);

    // 3. ClientGameSettings
    Zone_Packet_ClientGameSettings game_settings = {
        .interact_glow_and_dist = 16,
        .unk_bool         = TRUE,
        .timescale        = 1.0,
        .enable_weapons   = 1,
        .unk_u32_2        = 1,
        .unk_float2       = 15.,
        .damage_multiplier = 11.,
    };
    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_ClientGameSettings,
                   &game_settings);

    // 4. ReferenceData.DynamicAppearance — empty valid packet
    {
        u8 emptyDynAppearance[] = {
            0x17, 0x06,
            0x04, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        u8* baseBuffer = arena_push_size(&app->arenaPerTick, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
        memcpy(baseBuffer + TunnelDataHeaderLen, emptyDynAppearance, sizeof(emptyDynAppearance));
        GatewayTunnelDataSend(app, session, baseBuffer, sizeof(emptyDynAppearance) + TunnelDataHeaderLen);
    }

    // 5. SendSelfToClient
    printf("[DEBUG] characterName: '%.*s' len=%d\n",
           (int)session->characterName.size,
           session->characterName.data,
           (int)session->characterName.size);
    printf("[DEBUG] characterId: 0x%llx\n", (unsigned long long)session->characterId);
    printf("[DEBUG] actorModelId: %u gender: %u headType: %u\n",
           session->pGetPlayerActor.actorModelId,
           session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType);

    SendSelfToClient(app, session, FALSE);

    // 6. AddLightweightPc — broadcast self presence to proximity system
    // Zone_Packet_AddLightweightPc lightweightPc = { 0 };
    // lightweightPc.character_id          = session->characterId;
    // lightweightPc.transient_id.value    = 1;
    // lightweightPc.id_characterFirstName = session->characterName;
    // lightweightPc.id_characterLastName  = STR8("");
    // lightweightPc.id_unknownString1     = STR8("00000000000000000");
    // lightweightPc.id_characterName      = session->characterName;
    // lightweightPc.actorModelId          = session->pGetPlayerActor.actorModelId;
    // lightweightPc.position = (vec3){
    //     .x = -297.31f, .y = 506.06f, .z = -4894.10f
    // };
    // lightweightPc.rotation = (vec4){
    //     .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f
    // };
    // lightweightPc.movementVersion = 1;
    // lightweightPc.flags1          = 0;
    // ZonePacketSendDebug(app, session, &app->arenaPerTick,
    //                Zone_Packet_Kind_AddLightweightPc, &lightweightPc, "AddLightweightPc");

    // 7. Character.UpdateScale
    Zone_Packet_Character_UpdateScale updateScale = { 0 };
    updateScale.character_id = session->characterId;
    updateScale.scale = (vec4){ .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_Character_UpdateScale, &updateScale);

    // 8. Container.InitEquippedContainers
    Zone_Packet_ContainerInitEquippedContainers containers = { 0 };
    containers.character_id = session->characterId;
    containers.ignore_this  = 0;

    containers.container_list_count = 5;
    containers.container_list = (struct container_list_s[5]){
        [0] = {
            .loadout_slot_id        = 10,
            .guid_1                 = 0x1005,
            .defs_id                = 5747,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 5747,
                .item_defs_id_2     = 5747,
                .tint_id            = 0,
                .guid_2             = 0x1005,
                .count              = 1,
                .container_guid     = 0x1005,
                .contain_def_id     = 1,
                .container_slot_id  = 10,
                .base_durability    = 0,
                .current_durability = 0,
                .max_durability_from_defs = 0,
                .unk_bool_1         = FALSE,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
            .max_bulk       = 0,
            .bulk_used      = 0,
            .has_bulk_limit = FALSE,
        },
        [1] = {
            .loadout_slot_id        = 14,
            .guid_1                 = 0x1006,
            .defs_id                = 2178,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 2178,
                .item_defs_id_2     = 2178,
                .guid_2             = 0x1006,
                .count              = 1,
                .container_guid     = 0x1006,
                .contain_def_id     = 1,
                .container_slot_id  = 14,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [2] = {
            .loadout_slot_id        = 13,
            .guid_1                 = 0x1007,
            .defs_id                = 2216,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 2216,
                .item_defs_id_2     = 2216,
                .guid_2             = 0x1007,
                .count              = 1,
                .container_guid     = 0x1007,
                .contain_def_id     = 1,
                .container_slot_id  = 13,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [3] = {
            .loadout_slot_id        = 7,
            .guid_1                 = ITEM_GUID_FISTS,
            .defs_id                = 85,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 85,
                .item_defs_id_2     = 85,
                .guid_2             = ITEM_GUID_FISTS,
                .count              = 1,
                .container_guid     = ITEM_GUID_FISTS,
                .contain_def_id     = 1,
                .container_slot_id  = 7,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
        [4] = {
            .loadout_slot_id        = 5,
            .guid_1                 = ITEM_GUID_BINOCULARS,
            .defs_id                = 1542,
            .associated_character_id = session->characterId,
            .slots                  = 1,
            .items_list_count       = 1,
            .items_list = (struct items_list_s[1]){[0] = {
                .item_defs_id_1     = 1542,
                .item_defs_id_2     = 1542,
                .guid_2             = ITEM_GUID_BINOCULARS,
                .count              = 1,
                .container_guid     = ITEM_GUID_BINOCULARS,
                .contain_def_id     = 1,
                .container_slot_id  = 5,
                .owner_character_id = session->characterId,
            }},
            .show_bulk      = FALSE,
        },
    };

    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ContainerInitEquippedContainers, &containers);

    // 9. Reference data
    //ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(10), "..\\data\\Command_ItemDefinitions.bin");
    // Command.ItemDefinitions — empty list, no server-side item defs
    // Command.ItemDefinitions — 5 items: fists, binoculars, hoodie, jeans, sneakers
Zone_Packet_CommandItemDefinitions itemDefs = { 0 };

itemDefs.item_def_reply_2_length = 1;
itemDefs.item_def_reply_2 = (struct item_def_reply_2_s[1]){
    [0] = {
        .item_defs_count = 5,
        .item_defs = (struct item_defs_s[5]){
            [0] = {
                // Fists
                .defs_id       = 85,
                .bitflags1     = 0,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP | FLAG_NO_DRAG_DROP yes drop
                .name_id       = 0,
                .item_class    = 25006,
                .item_type     = 20,
                .item_type_1   = 20,
                .category_id   = 11,
                .model_name    = STR8("Weapon_Empty.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 0,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 43001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [1] = {
                // Binoculars
                .defs_id       = 1542,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25054,
                .item_type     = 20,
                .item_type_1   = 20,
                .category_id   = 16,
                .model_name    = STR8("Weapon_Binoculars_3P.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [2] = {
                // Gas Runner Hoodie
                .defs_id       = 5747,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25002,
                .item_type     = 34,
                .item_type_1   = 34,
                .category_id   = 1,
                .model_name    = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 55001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [3] = {
                // Jeans
                .defs_id       = 2178,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25003,
                .item_type     = 34,
                .item_type_1   = 34,
                .category_id   = 3,
                .model_name    = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 50,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 55001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
            [4] = {
                // Conveys Sneakers
                .defs_id       = 2216,
                .bitflags2     = 0b00000100, // FLAG_CAN_EQUIP
                .item_class    = 25005,
                .item_type     = 28,
                .item_type_1   = 28,
                .category_id   = 107,
                .model_name    = STR8("SurvivorMale_Feet_Conveys.adr"),
                .texture_alias = STR8(""),
                .tint_alias    = STR8(""),
                .bulk          = 200,
                .max_stack_size = 1,
                .min_stack_size = 1,
                .power_rating  = 49001,
                .curreny_type  = -1,
                .stats_item_def_2_count = 0,
            },
        },
    },
};

ZonePacketSend(app, session, &app->arenaPerTick,
               Zone_Packet_Kind_CommandItemDefinitions, &itemDefs);
    Zone_Packet_ReferenceDataWeaponDefinitions weaponDefs = { 0 };

weaponDefs.weapon_byteswithlength_length = 1;
weaponDefs.weapon_byteswithlength = (struct weapon_byteswithlength_s[1]){
    [0] = {
        .weapon_defs_count = 1,
        .weapon_defs = (struct weapon_defs_s[1]){
            [0] = {
                .id1                 = 85,
                .id2                 = 85,
                .weapon_group_id     = 0,
                .flags1              = 0,
                .equip_ms            = 500,
                .unequip_ms          = 500,
                .melee_detect_width  = 100,
                .melee_detect_height = 100,
                .anim_set_name       = STR8("Fists"),
                .ammo_slots_count    = 0,
                .fire_groups_count   = 1,
                .fire_groups = (struct fire_groups_s[1]){
                    [0] = { .fire_group_id = 1 },
                },
            },
        },
        .fire_group_defs_count = 1,
        .fire_group_defs = (struct fire_group_defs_s[1]){
            [0] = {
                .id3 = 1,
                .id4 = 1,
                .fire_mode_list_count = 1,
                .fire_mode_list = (struct fire_mode_list_s[1]){
                    [0] = { .fire_mode_1 = 1 },
                },
            },
        },
        .fire_mode_defs_count = 1,
        .fire_mode_defs = (struct fire_mode_defs_s[1]){
            [0] = {
                .id5            = 1,
                .id6            = 1,
                .type           = 1,
                .refire_time_ms = 500,
                .range          = 2.0f,
                // Third person camera
                .tp_force_camera_overrides  = TRUE,
                .tp_camera_distance         = 3.5f,
                .tp_cr_camera_distance      = 3.0f,
                .tp_pr_camera_distance      = 3.5f,
                .tp_camera_fov              = 75.0f,
                .tp_cr_camera_fov           = 75.0f,
                .tp_pr_camera_fov           = 75.0f,
                .fp_force_camera_overrides  = FALSE,
                .fp_camera_fov              = 250.0f,
            },
        },
        .player_state_group_defs_count          = 0,
        .fire_mode_projectile_mapping_data_count = 0,
        .aim_assist_defs_count                  = 0,
    },
};

ZonePacketSend(app, session, &app->arenaPerTick,
               Zone_Packet_Kind_ReferenceDataWeaponDefinitions, &weaponDefs);

    // Reset loading flags before zone transition
    session->finished_loading = FALSE;
    session->isReady          = FALSE;

    printf("[ONLOGIN] Flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
    printf("[ONLOGIN] Session actorModelId=%u gender=%u headType=%u headActor='%.*s'\n",
           session->pGetPlayerActor.actorModelId, session->pGetPlayerActor.gender,
           session->pGetPlayerActor.headType,
           (int)session->pGetPlayerActor.headActor.size, session->pGetPlayerActor.headActor.data);

    // 10. ClientBeginZoning
    // Zone_Packet_ClientBeginZoning beginZoning = { 0 };
    // beginZoning.zone_name  = STR8("Z2");
    // beginZoning.zone_type  = 4;
    // beginZoning.pos        = (vec4){ .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.0f };
    // beginZoning.rot        = (vec4){ .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f };
    // beginZoning.overcast   = 1.0f;
    // beginZoning.fogDensity = 0.000173f;
    // beginZoning.fogFloor   = 10.0f;
    // beginZoning.fogGradient = 0.0144f;
    // beginZoning.globalPrecipitation = 0.0f;
    // beginZoning.temperature = 75.0f;
    // beginZoning.skyClarity  = 0.0f;
    // beginZoning.cloudWeight0 = 0.05f;
    // beginZoning.cloudWeight1 = 0.0f;
    // beginZoning.cloudWeight2 = 0.05f;
    // beginZoning.cloudWeight3 = 0.15f;
    // beginZoning.transitionTime = 0.0f;
    // beginZoning.sunAxisX = 38.0f;
    // beginZoning.sunAxisY = -15.0f;
    // beginZoning.sunAxisZ = 0.0f;
    // beginZoning.windDirX = -1.0f;
    // beginZoning.windDirY = -0.5f;
    // beginZoning.windDirZ = -1.0f;
    // beginZoning.wind = 3.0f;
    // beginZoning.rainMinStrength       = 0.0f;
    // beginZoning.rainRampUpTimeSeconds = 1.0f;
    // beginZoning.cloudFile             = STR8("sky_Z_clouds.dds");
    // beginZoning.stratusCloudTiling    = 0.30f;
    // beginZoning.stratusCloudScrollU   = -0.002f;
    // beginZoning.stratusCloudScrollV   = 0.0f;
    // beginZoning.stratusCloudHeight    = 1000.0f;
    // beginZoning.cumulusCloudTiling    = 0.20f;
    // beginZoning.cumulusCloudScrollU   = 0.0f;
    // beginZoning.cumulusCloudScrollV   = 0.002f;
    // beginZoning.cumulusCloudHeight    = 8000.0f;
    // beginZoning.cloudAnimationSpeed   = 0.0f;
    // beginZoning.cloudSilverLiningThickness  = 0.25f;
    // beginZoning.cloudSilverLiningBrightness = 7.0f;
    // beginZoning.cloudShadows    = 0.5f;
    // beginZoning.unk_byte_1      = 4;
    // beginZoning.zone_id_1       = 5;
    // beginZoning.zone_id_2       = 5;
    // beginZoning.name_id         = 61609;
    // beginZoning.unk_dword_1     = 0x0f2b07d0;
    // beginZoning.unk_bool_1      = FALSE;
    // beginZoning.wait_for_zone_ready = FALSE;
    // beginZoning.unk_bool_2      = FALSE;
    // ZonePacketSend(app, session, &app->arenaPerTick,
    //                Zone_Packet_Kind_ClientBeginZoning, &beginZoning);

    // 11. UpdateLocation
    Zone_Packet_ClientUpdate_UpdateLocation updateLocation = {
        .position = { .x = -297.31f, .y = 506.06f, .z = -4894.10f, .w = 1.f },
        .rotation = { .x = 0.0f, .y = -0.7071f, .z = 0.0f, .w = 0.7071f },
        .trigger_loading_screen = TRUE,
        .unk_u8_1 = 0,
        .unk_bool = FALSE,
    };
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientUpdate_UpdateLocation, &updateLocation);

    // 12. ClientInitializationDetails
    Zone_Packet_ClientInitializationDetails initDetails = { 0 };
    initDetails.unk_u32_1 = 1;
    ZonePacketSend(app, session, &app->arenaPerTick,
                   Zone_Packet_Kind_ClientInitializationDetails, &initDetails);

    __time64_t tzEnd; _time64(&tzEnd);
    printf("[ONLOGIN] All init packets sent in %lld seconds, waiting for ClientIsReady\n", tzEnd - onLoginTime);
    printf("[ONLOGIN] Post-init flags: finished_loading=%d isReady=%d characterReleased=%d\n",
           session->finished_loading, session->isReady, session->characterReleased);
}