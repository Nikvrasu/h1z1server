u32 GetActorModelId(SessionState* session) {
    u32 headId = session->pGetPlayerActor.headType;

    switch (headId) {
        case 1: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 2: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 3: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 4: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 5: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 6: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 7: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 8: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        default: {
            return 0;
        }
    }
}

u32 GetGender(SessionState* session) {
    u32 actorModelId = session->pGetPlayerActor.actorModelId;

    switch (actorModelId) {
        case 9240: {
            session->pGetPlayerActor.gender = 1;
            return session->pGetPlayerActor.gender;
        } break;
        case 9474: {
            session->pGetPlayerActor.gender = 2;
            return session->pGetPlayerActor.gender;
        } break;
        default: {
            return 0;
        }
    }
}

char* GetHairModel(u32 actorModelId) {
    switch (actorModelId) {
        case 9240:
            printf("Male Hair Model Selected!\n");
            return "SurvivorMale_Hair_MediumMessy.adr";
        case 9474:
            printf("Female Hair Model Selected!\n");
            return "SurvivorFemale_Hair_ShortBun.adr";
        default:
            return "";
    }
}

u32 getResourceType(u32 resourceId) {
    switch (resourceId) {
        case HEALTHID:
            return HEALTHTYPE;
        case HUNGERID:
            return HUNGERTYPE;
        case HYDRATIONID:
            return HYDRATIONTYPE;
        case STAMINAID:
            return STAMINATYPE;
        case VIRUSID:
            return VIRUSTYPE;
        case BLEEDINGID:
            return BLEEDINGTYPE;
        case COMFORTID:
            return COMFORTTYPE;
        case FUELID:
            return FUELTYPE;
        case CONDITIONID:
            return CONDITIONTYPE;
        default:
            return 0;
    }
}

void SendSelfToClient(AppState* app, SessionState* session) {
    Zone_Packet_SendSelfToClient sendSelf = { 0 };

    sendSelf.payload_self = (struct payload_self_s[1]){
        [0] = {
            .character_id = session->characterId,
            .guid = session->characterId,
            .transient_id.value = 52,
            .position = {.x = -32.26f, .y = 506.41f, .z = 280.21f, .w = 1.f},
            .rotation = {.x = -0.11f, .y = -0.58f, .z = -0.08f, .w = 1.f},
            .head_id = 1,
            .actor_model_id = 9240,
            .gender1 = 1,
            .head_actor = STR8("SurvivorMale_Head_01.adr"),
            .hair_model = STR8("SurvivorMale_Hair_MediumMessy.adr"),
            .is_respawning = FALSE,
            // .character_name = session->characterName,
            .character_name = STR8("awdawd"),
            .loadout_id = 3,
            .current_slot_id = 7,
            .character_resources_count = 9,
            .character_resources = (struct character_resources_s[9]){
                [0] = {
                    .resource_type1 = HEALTHTYPE,
                    .resource_id = HEALTHID,
                    .resource_type2 = HEALTHTYPE,
                    .value = 10000,
                    .unk_array_22866_count = 0,
                },
                [1] = {
                    .resource_type1 = HUNGERTYPE,
                    .resource_id = HUNGERID,
                    .resource_type2 = HUNGERTYPE,
                    .value = 10000,
                    .unk_array_22866_count = 0,
                },
                [2] = {
                    .resource_type1 = HYDRATIONTYPE,
                    .resource_id = HYDRATIONID,
                    .resource_type2 = HYDRATIONTYPE,
                    .value = 10000,
                    .unk_array_22866_count = 0,
                },
                [3] = {
                    .resource_type1 = STAMINATYPE,
                    .resource_id = STAMINAID,
                    .resource_type2 = STAMINATYPE,
                    .value = 10000,
                    .unk_array_22866_count = 0,
                },
                [4] = {
                    .resource_type1 = VIRUSTYPE,
                    .resource_id = VIRUSID,
                    .resource_type2 = VIRUSTYPE,
                    .value = 0,
                    .unk_array_22866_count = 0,
                },
                [5] = {
                    .resource_type1 = BLEEDINGTYPE,
                    .resource_id = BLEEDINGID,
                    .resource_type2 = BLEEDINGTYPE,
                    .value = 0,
                    .unk_array_22866_count = 0,
                },
                [6] = {
                    .resource_type1 = COMFORTTYPE,
                    .resource_id = COMFORTID,
                    .resource_type2 = COMFORTTYPE,
                    .value = 5000,
                    .unk_array_22866_count = 0,
                },
                [7] = {
                    .resource_type1 = FUELTYPE,
                    .resource_id = FUELID,
                    .resource_type2 = FUELTYPE,
                    .value = 0,
                    .unk_array_22866_count = 0,
                },
                [8] = {
                    .resource_type1 = CONDITIONTYPE,
                    .resource_id = CONDITIONID,
                    .resource_type2 = CONDITIONTYPE,
                    .value = 10000,
                    .unk_array_22866_count = 0,
                },
            },
            .is_admin = TRUE,
        },
    };

    ZonePacketSend(app, session, &app->arenaPerTick, Zone_Packet_Kind_SendSelfToClient,
                   &sendSelf);
}