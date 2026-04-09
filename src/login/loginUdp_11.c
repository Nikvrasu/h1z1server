// ============================================================================
// Login Packet Send — structured packet serialization
// ============================================================================
void LoginPacketSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                     Login_Packet_Kind kind, void* packetPtr) {
    u8* dataBuffer = arena_push_size(arena, maxLen);
    u32 dataBufferLen = login_packet_pack(kind, packetPtr, dataBuffer);
    printf(MESSAGE_CONCAT_INFO("Sending %s (%u bytes)\n"), login_packet_names[kind], dataBufferLen);
    OutputStreamWrite(app, session, &session->outputStream, dataBuffer, dataBufferLen, FALSE);
}

// ============================================================================
// Login Packet Send — raw binary file (for cached/captured packets)
// ============================================================================
void LoginPacketRawFileSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                            char* path) {
    u8* baseBuffer = arena_push_size(arena, maxLen);
    u32 packedLen = app->api->buffer_load_from_file(path, baseBuffer, maxLen);
    arena_rewind(arena, maxLen - packedLen);
    OutputStreamWrite(app, session, &session->outputStream, baseBuffer, packedLen, FALSE);
}

// ============================================================================
// Head Type Lookup — maps head_type ID to actor model, gender, and assets
//
// Head type mapping from CharacterSelect.HeadTypes data table:
//   1 = Male Head 1   (SurvivorMale_Head_01.adr,   gender 1, model 9469)
//   2 = Male Head 2   (SurvivorMale_Head_02.adr,   gender 1, model 9469)
//   3 = Female Head 1 (SurvivorFemale_Head_01.adr, gender 2, model 9474)
//   4 = Female Head 2 (SurvivorFemale_Head_02.adr, gender 2, model 9474)
//   5 = Male Head 3   (SurvivorMale_Head_03.adr,   gender 1, model 9469)
//   6 = Female Head 3 (SurvivorFemale_Head_03.adr, gender 2, model 9474)
//   7 = Male Head 4   (SurvivorMale_Head_04.adr,   gender 1, model 9469)
//   8 = Female Head 4 (SurvivorFemale_Head_04.adr, gender 2, model 9474)
// ============================================================================
typedef struct HeadTypeInfo {
    u32 headType;
    u32 gender;      // 1=male, 2=female
    u32 actorModelId;
    String8 hairModel;
    String8 headActor;
} HeadTypeInfo;

// Returns TRUE if headId is valid, FALSE otherwise
b32 LookupHeadType(u32 headId, HeadTypeInfo* out) {
    // Male: headIds 1,2,5,7 → model 9469, gender 1
    // Female: headIds 3,4,6,8 → model 9474, gender 2
    static const struct { u32 id; u32 gender; u32 model; char* hair; char* head; } headTable[] = {
        { 1, 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",   "SurvivorMale_Head_01.adr"   },
        { 2, 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",   "SurvivorMale_Head_02.adr"   },
        { 3, 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",    "SurvivorFemale_Head_01.adr" },
        { 4, 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",    "SurvivorFemale_Head_02.adr" },
        { 5, 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",   "SurvivorMale_Head_03.adr"   },
        { 6, 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",    "SurvivorFemale_Head_03.adr" },
        { 7, 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",   "SurvivorMale_Head_04.adr"   },
        { 8, 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",    "SurvivorFemale_Head_04.adr" },
    };

    for (u32 i = 0; i < ARRAY_COUNT(headTable); i++) {
        if (headTable[i].id == headId) {
            out->headType     = headTable[i].id;
            out->gender       = headTable[i].gender;
            out->actorModelId = headTable[i].model;
            out->hairModel    = string8_make((u8*)headTable[i].hair, strlen(headTable[i].hair));
            out->headActor    = string8_make((u8*)headTable[i].head, strlen(headTable[i].head));
            return TRUE;
        }
    }
    return FALSE;
}

void SetPlayerActorFromHeadType(SessionState* session, u32 headId) {
    HeadTypeInfo info;
    if (LookupHeadType(headId, &info)) {
        session->pGetPlayerActor.headType     = info.headType;
        session->pGetPlayerActor.gender       = info.gender;
        session->pGetPlayerActor.actorModelId = info.actorModelId;
        session->pGetPlayerActor.hairModel    = info.hairModel;
        session->pGetPlayerActor.headActor    = info.headActor;
    } else {
        printf(MESSAGE_CONCAT_WARN("Invalid head type %u, defaulting to Male Head 1\n"), headId);
        session->pGetPlayerActor.headType     = 1;
        session->pGetPlayerActor.gender       = 1;
        session->pGetPlayerActor.actorModelId = 9469;
        session->pGetPlayerActor.hairModel    = STR8("SurvivorMale_Hair_MediumMessy.adr");
        session->pGetPlayerActor.headActor    = STR8("SurvivorMale_Head_01.adr");
    }
}

// ============================================================================
// Name Validation — validates character name via TunnelApp packet
// Rules: 3-20 chars, alphanumeric only. Status: 1=valid, 3=invalid
// Ref: H1emu loginserver.ts CharacterCreateRequest handler
// ============================================================================
void NameValidation(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_TunnelAppPacketClientToServer;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    i32 offset = sizeof(u8);
    Login_Packet_TunnelAppPacketClientToServer packet = { 0 };
    login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

    session->selected_server_id = packet.server_id;

    // Store character name in persistent arena
    u32 nameLen = packet.data_client->character_name.size;
    session->characterName.size = nameLen;
    session->characterName.data = arena_push_size(&app->arenaTotal, nameLen);
    memcpy(session->characterName.data, packet.data_client->character_name.data, nameLen);

    // Validate: 3-20 chars, alphanumeric only
    u32 validationStatus = 1; // 1=valid
    if (nameLen < 3 || nameLen > 20) {
        validationStatus = 3; // 3=invalid
    } else {
        for (u32 i = 0; i < nameLen; i++) {
            char c = session->characterName.data[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
                validationStatus = 3;
                break;
            }
        }
    }

    printf(MESSAGE_CONCAT_INFO("Name '%.*s' validation: %s\n"),
           (int)nameLen, session->characterName.data,
           validationStatus == 1 ? "VALID" : "INVALID");

    // Send validation result back through tunnel
    Login_Packet_TunnelAppPacketServerToClient packetReply = { 0 };
    packetReply.server_id = session->selected_server_id;
    packetReply.data_server_length = 14 + session->characterName.size;
    packetReply.data_server = (struct data_server_s[1]){{
        .tunnel_op_code = 0xa7,
        .sub_op_code    = 0x02,
        .character_name = session->characterName,
        .status         = validationStatus,
    }};

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_TunnelAppPacketServerToClient, &packetReply);
}

// ============================================================================
// Character Create — generates a new character ID and sends reply
// Ref: H1emu loginserver.ts CharacterCreateRequest handler
// ============================================================================
void CharacterCreate(AppState* app, SessionState* session) {
    session->characterId = generateRandomGuid();
    session->createReply.status = 1;

    Login_Packet_CharacterCreateReply packetReply = { 0 };
    packetReply.character_id = session->characterId;
    packetReply.status = 1;

    printf(MESSAGE_CONCAT_INFO("Created character 0x%llx '%.*s'\n"),
           (unsigned long long)session->characterId,
           (int)session->characterName.size, session->characterName.data);

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterCreateReply, &packetReply);
}

// ============================================================================
// Character Select Info — sends character list for the select screen
// Ref: H1emu loginserver.ts CharacterSelectInfoRequest handler
//
// Contains: character payload, loadout slots, item definitions
// The client uses this to render the character model on the select screen.
// ============================================================================
void CharacterSelectInfo(AppState* app, SessionState* session) {
    Login_Packet_CharacterSelectInfoReply packetReply = { 0 };

    packetReply.character_status = 1;
    packetReply.can_bypass_server_lock = TRUE;

    // Build character entry — one character per session
    packetReply.characters_count = 1;
    packetReply.characters = (struct characters_s[1]){{
        .charId        = session->characterId,
        .lastLoginDate = 0x00ull,
        .serverId      = session->selected_server_id,
        .status        = (session->createReply.status == 1) ? 1u : 0u,
    }};

    // Character payload — identity, appearance, profile
    packetReply.characters->payload = (struct payload_s[1]){{
        .name                  = session->characterName,
        .empireId              = 2,
        .battleRank            = 0,
        .nextBattleRankPercent = 0,
        .headId                = session->pGetPlayerActor.headType,
        .actorModelId          = session->pGetPlayerActor.actorModelId,
        .gender                = session->pGetPlayerActor.gender,
        .profileId             = 4,
        .unkDword1             = 0,
        .unkDword2             = 0,
        .lastUseDate           = 0,
    }};

    // Default loadout slots — fists + binoculars
    packetReply.characters->payload->loadoutSlots_count = 2;
    packetReply.characters->payload->loadoutSlots = (struct loadoutSlots_s[2]){
        [0] = {
            .hotbarSlotId    = LOADOUT_SLOT_MELEE,
            .loadoutId       = LOADOUT_ID_KOTK_CHARACTER,
            .slotId          = LOADOUT_SLOT_MELEE,
            .itemDefId       = WEAPON_FISTS,
            .loadoutItemGuid = ITEM_GUID_FISTS,
            .unkByte1        = 1,
            .unkDword1       = 22,
        },
        [1] = {
            .hotbarSlotId    = LOADOUT_SLOT_BINOCULARS,
            .loadoutId       = LOADOUT_ID_KOTK_CHARACTER,
            .slotId          = LOADOUT_SLOT_BINOCULARS,
            .itemDefId       = WEAPON_BINOCULARS,
            .loadoutItemGuid = ITEM_GUID_BINOCULARS,
            .unkByte1        = 1,
            .unkDword1       = 22,
        },
    };

    // Item definitions — needed for the character select screen to display items
    packetReply.characters->payload->itemDefinitions_count = 1;
    packetReply.characters->payload->itemDefinitions = (struct itemDefinitions_s[1]){
        [0] = {
            .ID = 0,
            .item_defs_count = 5,
            .item_defs = (struct item_defs_s[5]){
                [0] = { // Fists
                    .defs_id       = 85,
                    .bitflags2     = 0b00001100,
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
                [1] = { // Binoculars
                    .defs_id       = 1542,
                    .bitflags2     = 0b00000100,
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
                [2] = { // Gas Runner Hoodie (chest)
                    .defs_id       = 5747,
                    .bitflags2     = 0b00000100,
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
                [3] = { // Jeans (legs)
                    .defs_id       = 2178,
                    .bitflags2     = 0b00000100,
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
                [4] = { // Conveys Sneakers (feet)
                    .defs_id       = 2216,
                    .bitflags2     = 0b00000100,
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

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterSelectInfoReply, &packetReply);
}
