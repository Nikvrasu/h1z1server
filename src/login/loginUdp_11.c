// #############################################//
// Send structured packet data via this function//
// #############################################//
void LoginPacketSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                     Login_Packet_Kind kind, void* packetPtr) {
    u8* dataBuffer = arena_push_size(arena, maxLen);
    u32 dataBufferLen = login_packet_pack(kind, packetPtr, dataBuffer);

    OutputStreamWrite(app, session, &session->outputStream, dataBuffer, dataBufferLen, FALSE);
}

// ############################################//
//    Send raw packet data via this function   //
// ############################################//
void LoginPacketRawFileSend(AppState* app, SessionState* session, Arena* arena, u32 maxLen,
                            char* path) {
    u8* baseBuffer = arena_push_size(arena, maxLen);

    u32 packedLen = app->api->buffer_load_from_file(path, baseBuffer, maxLen);
    u32 totalLen = packedLen;

    arena_rewind(arena, maxLen - totalLen);
    OutputStreamWrite(app, session, &session->outputStream, baseBuffer, packedLen, FALSE);
}

// #######################################################//
// Validates user's character name, 1 = valid 3 = invalid //
// #######################################################//
void NameValidation(AppState* app, SessionState* session, u8* data, u32 dataLen) {
    Login_Packet_Kind kind = Login_Packet_Kind_TunnelAppPacketClientToServer;
    printf("Received %s\n", login_packet_names[kind]);

    i32 offset = sizeof(u8);

    Login_Packet_TunnelAppPacketClientToServer packet = { 0 };
    login_packet_unpack(data + offset, dataLen - offset, kind, &packet, &app->arenaPerTick);

    u32 validationStatus = 1;
    u32 nameLen = packet.data_client->character_name.size;

    session->selected_server_id = packet.server_id;

    // allocate into arenaTotal so it persists across ticks
    session->characterName.size = nameLen;
    session->characterName.data = arena_push_size(&app->arenaTotal, nameLen);
    memcpy(session->characterName.data,
           packet.data_client->character_name.data,
           nameLen);

    printf("[DEBUG] NameValidation stored name: '%.*s' len=%d\n",
           (int)nameLen, session->characterName.data, (int)nameLen);

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
}

// ############################################//
//   Getter for the character's head type ID   //
// ############################################//
void GetHeadTypeId(SessionState* session, void* packetPtr) {
    Login_Packet_CharacterCreateRequest* characterCreateReq = packetPtr;
    u32 headId = characterCreateReq->char_payload->head_type;

    // Head type mapping from CharacterSelect.HeadTypes data table:
    //   1 = Male Head 1   (SurvivorMale_Head_01.adr,   gender 1, model 9469)
    //   2 = Male Head 2   (SurvivorMale_Head_02.adr,   gender 1, model 9469)
    //   3 = Female Head 1 (SurvivorFemale_Head_01.adr, gender 2, model 9474)
    //   4 = Female Head 2 (SurvivorFemale_Head_02.adr, gender 2, model 9474)
    //   5 = Male Head 3   (SurvivorMale_Head_03.adr,   gender 1, model 9469)
    //   6 = Female Head 3 (SurvivorFemale_Head_03.adr, gender 2, model 9474)
    //   7 = Male Head 4   (SurvivorMale_Head_04.adr,   gender 1, model 9469)
    //   8 = Female Head 4 (SurvivorFemale_Head_04.adr, gender 2, model 9474)
    switch (headId) {
        case 1: {
            session->pGetPlayerActor.headType = 1;
            session->pGetPlayerActor.gender = 1;
            session->pGetPlayerActor.actorModelId = 9469;
            session->pGetPlayerActor.hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorMale_Head_01.adr");
        } break;
        case 2: {
            session->pGetPlayerActor.headType = 2;
            session->pGetPlayerActor.gender = 1;
            session->pGetPlayerActor.actorModelId = 9469;
            session->pGetPlayerActor.hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorMale_Head_02.adr");
        } break;
        case 3: {
            session->pGetPlayerActor.headType = 3;
            session->pGetPlayerActor.gender = 2;
            session->pGetPlayerActor.actorModelId = 9474;
            session->pGetPlayerActor.hairModel = STR8("SurvivorFemale_Hair_ShortBun.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorFemale_Head_01.adr");
        } break;
        case 4: {
            session->pGetPlayerActor.headType = 4;
            session->pGetPlayerActor.gender = 2;
            session->pGetPlayerActor.actorModelId = 9474;
            session->pGetPlayerActor.hairModel = STR8("SurvivorFemale_Hair_ShortBun.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorFemale_Head_02.adr");
        } break;
        case 5: {
            session->pGetPlayerActor.headType = 5;
            session->pGetPlayerActor.gender = 1;
            session->pGetPlayerActor.actorModelId = 9469;
            session->pGetPlayerActor.hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorMale_Head_03.adr");
        } break;
        case 6: {
            session->pGetPlayerActor.headType = 6;
            session->pGetPlayerActor.gender = 2;
            session->pGetPlayerActor.actorModelId = 9474;
            session->pGetPlayerActor.hairModel = STR8("SurvivorFemale_Hair_ShortBun.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorFemale_Head_03.adr");
        } break;
        case 7: {
            session->pGetPlayerActor.headType = 7;
            session->pGetPlayerActor.gender = 1;
            session->pGetPlayerActor.actorModelId = 9469;
            session->pGetPlayerActor.hairModel = STR8("SurvivorMale_Hair_MediumMessy.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorMale_Head_04.adr");
        } break;
        case 8: {
            session->pGetPlayerActor.headType = 8;
            session->pGetPlayerActor.gender = 2;
            session->pGetPlayerActor.actorModelId = 9474;
            session->pGetPlayerActor.hairModel = STR8("SurvivorFemale_Hair_ShortBun.adr");
            session->pGetPlayerActor.headActor = STR8("SurvivorFemale_Head_04.adr");
        } break;
        default: {
            printf("Head type data is invalid! headId=%u\n", headId);
            return;
        }
    }
}

// ############################################//
// Character create function, self explanatory //
// ############################################//
void CharacterCreate(AppState* app, SessionState* session) {
    session->characterId = generateRandomGuid();

    Login_Packet_CharacterCreateReply packetReply = { 0 };
    packetReply.character_id = session->characterId;
    packetReply.status = 1;

    session->createReply.status = packetReply.status;

    LoginPacketSend(app, session, &app->arenaPerTick, KB(10), Login_Packet_Kind_CharacterCreateReply,
                    &packetReply);
}

// ########################################################//
// Selected character function after you create a character//
// ########################################################//
void CharacterSelectInfo(AppState* app, SessionState* session) {
    Login_Packet_CharacterSelectInfoReply packetReply = { 0 };

    packetReply.character_status = 1;
    packetReply.can_bypass_server_lock = TRUE;

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
            .profileId             = KOTK_CHARACTER_PROFILE_ID,
            .unkDword1             = 0,
            .unkDword2             = 0,
            .lastUseDate           = 0,
        },
    };

    packetReply.characters->payload->loadoutSlots_count = 2;
    packetReply.characters->payload->loadoutSlots = (struct loadoutSlots_s[2]){
        {
            .hotbarSlotId    = LOADOUT_SLOT_MELEE,
            .loadoutId       = LOADOUT_ID_KOTK_CHARACTER,
            .slotId          = LOADOUT_SLOT_MELEE,
            .itemDefId       = WEAPON_FISTS,
            .loadoutItemGuid = ITEM_GUID_FISTS,
            .unkByte1        = 1,
            .unkDword1       = 22,
        },
        {
            .hotbarSlotId    = LOADOUT_SLOT_BINOCULARS,
            .loadoutId       = LOADOUT_ID_KOTK_CHARACTER,
            .slotId          = LOADOUT_SLOT_BINOCULARS,
            .itemDefId       = WEAPON_BINOCULARS,
            .loadoutItemGuid = ITEM_GUID_BINOCULARS,
            .unkByte1        = 1,
            .unkDword1       = 22,
        },
    };

    packetReply.characters->payload->itemDefinitions_count = 1;
    packetReply.characters->payload->itemDefinitions = (struct itemDefinitions_s[1]){
        [0] = {
            .ID = 0,
            .item_defs_count = 6,
            .item_defs = (struct item_defs_s[6]){
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
                    .active_equip_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
                    .passive_equip_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
                    .passive_equip_slot_group_id = 0,
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
                    .active_equip_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
                    .passive_equip_slot_id = EQUIPMENT_SLOT_RIGHT_HAND,
                    .passive_equip_slot_group_id = 0,
                    .max_stack_size = 1,
                    .min_stack_size = 1,
                    .curreny_type  = -1,
                    .stats_item_def_2_count = 0,
                },
                [2] = {
                    .defs_id       = ITEM_DEF_HOODIE,
                    .bitflags2     = 0b00000100,
                    .item_class    = 25002,
                    .item_type     = 34,
                    .item_type_1   = 34,
                    .category_id   = 1,
                    .model_name    = STR8("SurvivorMale_Chest_Hoodie_Down.adr"),
                    .texture_alias = STR8(""),
                    .tint_alias    = STR8(""),
                    .bulk          = 50,
                    .active_equip_slot_id = EQUIPMENT_SLOT_CHEST,
                    .passive_equip_slot_id = EQUIPMENT_SLOT_CHEST,
                    .passive_equip_slot_group_id = 0,
                    .max_stack_size = 1,
                    .min_stack_size = 1,
                    .power_rating  = 55001,
                    .curreny_type  = -1,
                    .stats_item_def_2_count = 0,
                },
                [3] = {
                    .defs_id       = ITEM_DEF_SKINNY_JEANS,
                    .bitflags2     = 0b00000100,
                    .item_class    = 25003,
                    .item_type     = 34,
                    .item_type_1   = 34,
                    .category_id   = 3,
                    .model_name    = STR8("SurvivorMale_Legs_Pants_SkinnyLeg.adr"),
                    .texture_alias = STR8(""),
                    .tint_alias    = STR8(""),
                    .bulk          = 50,
                    .active_equip_slot_id = 4,
                    .passive_equip_slot_id = 4,
                    .passive_equip_slot_group_id = 0,
                    .max_stack_size = 1,
                    .min_stack_size = 1,
                    .power_rating  = 55001,
                    .curreny_type  = -1,
                    .stats_item_def_2_count = 0,
                },
                [4] = {
                    .defs_id       = ITEM_DEF_CONVEYS,
                    .bitflags2     = 0b00000100,
                    .item_class    = 25005,
                    .item_type     = 28,
                    .item_type_1   = 28,
                    .category_id   = 107,
                    .model_name    = STR8("SurvivorMale_Feet_Conveys.adr"),
                    .texture_alias = STR8(""),
                    .tint_alias    = STR8(""),
                    .bulk          = 200,
                    .active_equip_slot_id = 5,
                    .passive_equip_slot_id = 5,
                    .passive_equip_slot_group_id = 0,
                    .max_stack_size = 1,
                    .min_stack_size = 1,
                    .power_rating  = 49001,
                    .curreny_type  = -1,
                    .stats_item_def_2_count = 0,
                },
                [5] = {
                    .defs_id       = ITEM_DEF_EYES_ATTACHMENT,
                    .bitflags2     = 0b00000100,
                    .item_class    = 25004,
                    .item_type     = 34,
                    .item_type_1   = 34,
                    .category_id   = 2,
                    .model_name    = STR8("SurvivorMale_Eyes_01.adr"),
                    .texture_alias = STR8(""),
                    .tint_alias    = STR8(""),
                    .bulk          = 0,
                    .active_equip_slot_id = LOADOUT_SLOT_REQUIRED_EYES,
                    .passive_equip_slot_id = LOADOUT_SLOT_REQUIRED_EYES,
                    .passive_equip_slot_group_id = LOADOUT_SLOT_REQUIRED_EYES,
                    .max_stack_size = 1,
                    .min_stack_size = 1,
                    .power_rating  = 0,
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
}
