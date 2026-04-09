// ============================================================================
// Login Protocol Layer (LoginUdp_11)
//
// This file implements the login protocol API for KotK Preseason 3.
// Based on H1emu/h1z1-server LoginServer implementation.
//
// Packet flow (client → server → client):
//   1. LoginRequest        → LoginReply
//   2. CharacterSelectInfoRequest → CharacterSelectInfoReply
//   3. ServerListRequest   → ServerListReply
//   4. TunnelAppPacket     → TunnelAppPacket (name validation)
//   5. CharacterCreateRequest → CharacterCreateReply
//   6. CharacterLoginRequest  → CharacterLoginReply (with zone server ticket)
//   7. CharacterDeleteRequest → CharacterDeleteReply
//   8. Logout              → (session cleanup)
// ============================================================================

// ============================================================================
// LoginPacketSend — Serialize and send a structured login packet
// ============================================================================
void LoginPacketSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                     Login_Packet_Kind kind, void* packetPtr) {
    u8* dataBuffer = arena_push_size(arena, maxLen);
    u32 dataBufferLen = login_packet_pack(kind, packetPtr, dataBuffer);

    OutputStreamWrite(app, session, &session->outputStream, dataBuffer, dataBufferLen, FALSE);
}

// ============================================================================
// LoginPacketRawFileSend — Send a pre-captured binary packet from disk
// ============================================================================
void LoginPacketRawFileSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                            char* path) {
    u8* baseBuffer = arena_push_size(arena, maxLen);

    u32 packedLen = app->api->buffer_load_from_file(path, baseBuffer, maxLen);
    u32 totalLen = packedLen;

    arena_rewind(arena, maxLen - totalLen);
    OutputStreamWrite(app, session, &session->outputStream, baseBuffer, packedLen, FALSE);
}

// ============================================================================
// Character Model Data — Maps head type ID to actor model, gender, hair, head
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

typedef struct CharacterModelData {
    u32 headType;
    u32 gender;       // 1 = male, 2 = female
    u32 actorModelId; // 9469 = male, 9474 = female
    String8 hairModel;
    String8 headActor;
} CharacterModelData;

// Static lookup table for all 8 head types
static const struct {
    u32 gender;
    u32 actorModelId;
    char* hairModel;
    char* headActor;
} HEAD_TYPE_TABLE[] = {
    { 0, 0,    "",                                   ""                              }, // 0: invalid
    { 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",  "SurvivorMale_Head_01.adr"      }, // 1
    { 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",  "SurvivorMale_Head_02.adr"      }, // 2
    { 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",   "SurvivorFemale_Head_01.adr"    }, // 3
    { 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",   "SurvivorFemale_Head_02.adr"    }, // 4
    { 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",  "SurvivorMale_Head_03.adr"      }, // 5
    { 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",   "SurvivorFemale_Head_03.adr"    }, // 6
    { 1, 9469, "SurvivorMale_Hair_MediumMessy.adr",  "SurvivorMale_Head_04.adr"      }, // 7
    { 2, 9474, "SurvivorFemale_Hair_ShortBun.adr",   "SurvivorFemale_Head_04.adr"    }, // 8
};
#define HEAD_TYPE_COUNT (sizeof(HEAD_TYPE_TABLE) / sizeof(HEAD_TYPE_TABLE[0]))

b32 GetCharacterModelData(SessionState* session, u32 headId) {
    if (headId == 0 || headId >= HEAD_TYPE_COUNT) {
        printf(MESSAGE_CONCAT_WARN("Invalid head type %u\n"), headId);
        return FALSE;
    }

    session->pGetPlayerActor.headType     = headId;
    session->pGetPlayerActor.gender       = HEAD_TYPE_TABLE[headId].gender;
    session->pGetPlayerActor.actorModelId = HEAD_TYPE_TABLE[headId].actorModelId;
    session->pGetPlayerActor.hairModel    = string8_make(
        (u8*)HEAD_TYPE_TABLE[headId].hairModel,
        strlen(HEAD_TYPE_TABLE[headId].hairModel));
    session->pGetPlayerActor.headActor    = string8_make(
        (u8*)HEAD_TYPE_TABLE[headId].headActor,
        strlen(HEAD_TYPE_TABLE[headId].headActor));

    return TRUE;
}

// ============================================================================
// NameValidation — Validates character name via TunnelAppPacket
//
// H1emu flow: TunnelAppPacketClientToServer contains character_name to validate.
// Returns TunnelAppPacketServerToClient with status 1 (valid) or 3 (invalid).
// Name rules: 3-20 chars, alphanumeric only.
// ============================================================================
void NameValidation(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_TunnelAppPacketClientToServer;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    i32 offset = sizeof(u8);

    Login_Packet_TunnelAppPacketClientToServer packet = { 0 };
    login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

    u32 validationStatus = 1; // 1 = valid, 3 = invalid
    u32 nameLen = packet.data_client->character_name.size;

    session->selected_server_id = packet.server_id;

    // Persist character name across ticks (allocate into arenaTotal)
    session->characterName.size = nameLen;
    session->characterName.data = arena_push_size(&app->arenaTotal, nameLen);
    memcpy(session->characterName.data,
           packet.data_client->character_name.data,
           nameLen);

    printf(MESSAGE_CONCAT_INFO("Validating name: '%.*s' (len=%u)\n"),
           (int)nameLen, session->characterName.data, nameLen);

    // Validate: length 3-20, alphanumeric only
    if (nameLen < 3 || nameLen > 20) {
        validationStatus = 3;
    } else {
        for (u32 i = 0; i < nameLen; i++) {
            char c = session->characterName.data[i];
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z')) {
                validationStatus = 3;
                break;
            }
        }
    }

    // Send validation result back through tunnel
    Login_Packet_TunnelAppPacketServerToClient packetReply = { 0 };
    packetReply.server_id = session->selected_server_id;
    packetReply.data_server_length = 14 + session->characterName.size;
    packetReply.data_server = (struct data_server_s[1]){
        {
            .tunnel_op_code = 0xa7,
            .sub_op_code = 0x02,
            .character_name = session->characterName,
            .status = validationStatus,
        },
    };

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_TunnelAppPacketServerToClient, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Name validation result: %s\n"),
           validationStatus == 1 ? "VALID" : "INVALID");
}

// ============================================================================
// LoginReplyHandler — Handles LoginRequest and sends LoginReply
//
// H1emu flow:
//   Client sends LoginRequest with sessionId (JSON), locale, fingerprint.
//   Server responds with LoginReply containing:
//     - loggedIn, status, resultCode
//     - isMember, isInternal, namespace
//     - accountFeatures array
//     - ipCountryCode
//     - applicationPayload
// ============================================================================
void LoginReplyHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_LoginRequest;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    Login_Packet_LoginRequest packet = { 0 };
    login_packet_unpack(data + sizeof(u8), dataLen - sizeof(u8), kind, &packet, &app->arenaPerTick);

    // Build structured LoginReply matching H1emu's response format
    Login_Packet_LoginReply packetReply = { 0 };
    packetReply.is_logged_in = 1;
    packetReply.status = 1;
    packetReply.result_code = 1;
    packetReply.is_member = 1;
    packetReply.is_internal = 1;

    // Account features — H1emu sends one feature entry with key=2
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

    printf(MESSAGE_CONCAT_INFO("Sent LoginReply (loggedIn=1, status=1)\n"));
}

// ============================================================================
// ServerListReplyHandler — Handles ServerListRequest and sends server list
//
// H1emu flow:
//   Client requests available game servers.
//   Server responds with array of servers, each containing:
//     - id, state (2=online), name, description
//     - populationData XML, serverInfo XML
//     - is_access_allowed flag
// ============================================================================
void ServerListReplyHandler(AppState* app, SessionState* session) {
    printf(MESSAGE_CONCAT_INFO("Received ServerListRequest\n"));

    Login_Packet_ServerListReply packetReply = { 0 };

    packetReply.servers_count = 1;
    packetReply.servers = (struct servers_s[1]){
        {
            .id = 1,
            .state = 2, // 2 = online
            .is_locked = FALSE,
            .name = STR8("H1Z1-C-Server"),
            .name_id = 193,
            .description =
                STR8("A server-emulator for H1Z1: King of the Kill, Preseason 3; Built in C"),
            .description_id = 1362,
            .server_info = STR8(
                "<ServerInfo Region=\"CharacterCreate.RegionUs\" "
                "Subregion=\"UI.SubregionUS\" "
                "IsRecommended=\"1\" IsRecommendedVS=\"0\" IsRecommendedNC=\"0\" "
                "IsRecommendedTR=\"0\" />"),
            .population_data =
                STR8("<Population PctCap=\"0\" PingAdr=\"127.0.0.1:60000\" Rulesets=\"\" "
                     "Mode=\"13\" "
                     "IsLogin=\"1\" IsWL=\"0\" IsEvt=\"0\" PL=\"0\" DC=\"LVS\" PopLock=\"0\" "
                     "GP=\"100\" BP=\"175\" MaxPop=\"4000\" Subregion=\"US\">"
                     "<Fac IsList=\"1\"/></Population>"),
            .is_access_allowed = TRUE,
        },
    };

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_ServerListReply, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Sent ServerListReply (1 server)\n"));
}

// ============================================================================
// CharacterSelectInfoHandler — Sends character list for the select screen
//
// H1emu flow:
//   Client requests character list for account.
//   Server responds with status, canBypassServerLock, and character array.
//   Each character has: id, serverId, lastLoginDate, status, payload (name,
//   empireId, battleRank, headId, modelId, gender, profileId, loadoutSlots,
//   itemDefinitions, lastUseDate).
// ============================================================================
void CharacterSelectInfoHandler(AppState* app, SessionState* session) {
    printf(MESSAGE_CONCAT_INFO("Received CharacterSelectInfoRequest\n"));

    Login_Packet_CharacterSelectInfoReply packetReply = { 0 };

    packetReply.character_status = 1;
    packetReply.can_bypass_server_lock = TRUE;

    // If no character has been created yet, send empty list
    if (session->characterId == 0 && session->createReply.status != 1) {
        packetReply.characters_count = 0;
        LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                        Login_Packet_Kind_CharacterSelectInfoReply, &packetReply);
        printf(MESSAGE_CONCAT_INFO("Sent CharacterSelectInfoReply (0 characters)\n"));
        return;
    }

    packetReply.characters_count = 1;
    packetReply.characters = (struct characters_s[1]){
        {
            .charId        = session->characterId,
            .lastLoginDate = 0x00ull,
            .serverId      = session->selected_server_id,
        },
    };

    packetReply.characters->payload = (struct payload_s[1]){
        {
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
        },
    };

    // Default loadout slot — fists in melee slot
    packetReply.characters->payload->loadoutSlots_count = 1;
    packetReply.characters->payload->loadoutSlots = (struct loadoutSlots_s[1]){
        {
            .hotbarSlotId    = LOADOUT_SLOT_MELEE,
            .loadoutId       = LOADOUT_ID_KOTK_CHARACTER,
            .slotId          = LOADOUT_SLOT_MELEE,
            .itemDefId       = WEAPON_FISTS,
            .loadoutItemGuid = ITEM_GUID_FISTS,
            .unkByte1        = 1,
            .unkDword1       = 22,
        },
    };

    // Item definitions for the character select screen
    // Includes: fists, binoculars, hoodie, jeans, sneakers
    packetReply.characters->payload->itemDefinitions_count = 1;
    packetReply.characters->payload->itemDefinitions = (struct itemDefinitions_s[1]){
        [0] = {
            .ID = 0,
            .item_defs_count = 5,
            .item_defs = (struct item_defs_s[5]){
                [0] = {
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
                [1] = {
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
                [2] = {
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
                [3] = {
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
                [4] = {
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

    if (session->createReply.status == 1) {
        packetReply.characters->status = 1;
    }

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterSelectInfoReply, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Sent CharacterSelectInfoReply (1 character)\n"));
}

// ============================================================================
// CharacterDeleteHandler — Deletes a character and sends confirmation
//
// H1emu flow:
//   Client sends character_id to delete.
//   Server marks character as deleted (status=0) and responds with status=1.
// ============================================================================
void CharacterDeleteHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_CharacterDeleteRequest;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    Login_Packet_CharacterDeleteRequest packet = { 0 };
    login_packet_unpack(data + sizeof(u8), dataLen - sizeof(u8), kind, &packet, &app->arenaPerTick);

    Login_Packet_CharacterDeleteReply packetReply = { 0 };
    packetReply.character_id = session->characterId;
    packetReply.status = 1;

    // Reset character state on delete
    session->characterId = 0;
    session->createReply.status = 0;

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterDeleteReply, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Sent CharacterDeleteReply (status=1)\n"));
}

// ============================================================================
// CharacterCreateHandler — Creates a new character
//
// H1emu flow:
//   Client sends CharacterCreateRequest with serverId and payload
//   containing empireId, headType, profileType, gender, characterName.
//   Server generates a random GUID, stores character model data,
//   and responds with CharacterCreateReply containing the new character_id.
// ============================================================================
void CharacterCreateHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_CharacterCreateRequest;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    Login_Packet_CharacterCreateRequest packet = { 0 };
    login_packet_unpack(data + sizeof(u8), dataLen - sizeof(u8), kind, &packet, &app->arenaPerTick);

    // Resolve head type to model data (gender, actor model, hair, head actor)
    u32 headId = packet.char_payload->head_type;
    if (!GetCharacterModelData(session, headId)) {
        printf(MESSAGE_CONCAT_WARN("CharacterCreate failed: invalid head type %u\n"), headId);
        return;
    }

    // Generate unique character ID
    session->characterId = generateRandomGuid();

    Login_Packet_CharacterCreateReply packetReply = { 0 };
    packetReply.character_id = session->characterId;
    packetReply.status = 1;

    session->createReply.status = packetReply.status;

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterCreateReply, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Created character 0x%llx (head=%u, gender=%u, model=%u)\n"),
           (unsigned long long)session->characterId, headId,
           session->pGetPlayerActor.gender, session->pGetPlayerActor.actorModelId);
}

// ============================================================================
// CharacterLoginHandler — Handles character login and provides zone ticket
//
// H1emu flow:
//   Client sends CharacterLoginRequest with character_id and server_id.
//   Server responds with CharacterLoginReply containing:
//     - status (1 = allowed)
//     - login_payload with server_address, server_ticket, encryption_key,
//       soe_protocol_version, character_id, character_name
//   The server_ticket embeds identity:name for the zone to extract.
// ============================================================================
void CharacterLoginHandler(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_CharacterLoginRequest;
    printf(MESSAGE_CONCAT_INFO("Received %s\n"), login_packet_names[kind]);

    Login_Packet_CharacterLoginRequest packet = { 0 };
    login_packet_unpack(data + sizeof(u8), dataLen - sizeof(u8), kind, &packet, &app->arenaPerTick);

    Login_Packet_CharacterLoginReply packetReply = { 0 };
    packetReply.character_id = packet.character_id;
    packetReply.server_id = packet.server_id;
    packetReply.status = 1;

    // Build server ticket: "steamLikeId:CharacterName"
    // The zone server extracts the identity and name from this format
    char ticketBuf[256];
    u64 steamLikeId = 76561197960265728ull + (packet.character_id & 0xffffffffull);
    snprintf(ticketBuf, sizeof(ticketBuf), "%llu:%.*s",
             (unsigned long long)steamLikeId,
             (int)session->characterName.size,
             session->characterName.data);

    packetReply.login_payload = (struct login_payload_s[1]){
        {
            .server_address = STR8("127.0.0.1:60000"),
            .server_ticket = string8_make((u8*)ticketBuf, strlen(ticketBuf)),
            .encryption_key =
                STR8("\x17\xbd\x08\x6b\x1b\x94\xf0\x2f\xf0\xec\x53\xd7\x63\x58\x9b\x5f"),
            .soe_protocol_version = 3,
            .character_id = packet.character_id,
            .character_name = session->characterName,
        },
    };

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10),
                    Login_Packet_Kind_CharacterLoginReply, &packetReply);

    printf(MESSAGE_CONCAT_INFO("Sent CharacterLoginReply (charId=0x%llx → zone 127.0.0.1:60000)\n"),
           (unsigned long long)packet.character_id);
}
