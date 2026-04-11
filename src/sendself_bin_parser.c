#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SENDSELF_OPCODE 0x03

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

typedef struct LoadedFile {
    u8* bytes;
    u32 size;
} LoadedFile;

typedef struct ClassEntry {
    u32 float_offset;
    u32 code_at_minus_24;
    u32 code_at_minus_8;
    u32 code_at_plus_0;
    u32 v2_bits;
    u32 v3_bits;
    f32 v2;
    f32 v3;
    u32 pre_u32[6];
} ClassEntry;

typedef struct ParseSummary {
    const char* path;
    u32 size;
    int has_sendself_opcode;
    u32 stream_length_prefix;
    u32 stream_payload_bytes;
    u64 guid;
    u64 character_id;
    u32 candidate_list_count_offset;
    u32 candidate_list_count_value;
    int has_candidate_list_count;
    u32 confidence_170_095;
    u32 entry_count;
    ClassEntry entries[512];
} ParseSummary;

static LoadedFile load_file_bytes(const char* path) {
    LoadedFile result = { 0 };

    FILE* file = fopen(path, "rb");
    if (!file) {
        return result;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return result;
    }

    long size = ftell(file);
    if (size <= 0) {
        fclose(file);
        return result;
    }
    rewind(file);

    result.bytes = (u8*)malloc((size_t)size);
    if (!result.bytes) {
        fclose(file);
        return result;
    }

    if (fread(result.bytes, 1, (size_t)size, file) != (size_t)size) {
        free(result.bytes);
        result.bytes = 0;
        result.size = 0;
        fclose(file);
        return result;
    }

    fclose(file);
    result.size = (u32)size;
    return result;
}

static void free_loaded_file(LoadedFile* file) {
    if (file && file->bytes) {
        free(file->bytes);
        file->bytes = 0;
        file->size = 0;
    }
}

static u32 read_u32_le(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u64 read_u64_le(const u8* p) {
    return (u64)p[0] | ((u64)p[1] << 8) | ((u64)p[2] << 16) | ((u64)p[3] << 24)
         | ((u64)p[4] << 32) | ((u64)p[5] << 40) | ((u64)p[6] << 48) | ((u64)p[7] << 56);
}

static f32 bits_to_f32(u32 bits) {
    union {
        u32 u;
        f32 f;
    } v;

    v.u = bits;
    return v.f;
}

static int near_f32(f32 a, f32 b, f32 eps) {
    f32 d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= eps;
}

static void hexdump_window(const u8* bytes, u32 size, u32 start, u32 count) {
    u32 end = start + count;
    if (end > size) {
        end = size;
    }

    for (u32 i = start; i < end; i++) {
        if ((i - start) % 16 == 0) {
            printf("%08u: ", i);
        }
        printf("%02X ", bytes[i]);
        if ((i - start) % 16 == 15 || i + 1 == end) {
            printf("\n");
        }
    }
}

static u32 scan_class_entries(const u8* bytes, u32 size, ClassEntry* out, u32 max_out) {
    const u32 want_v2 = 0x3FD9999A; // 1.70
    const u32 want_v3 = 0x3F733333; // 0.95

    u32 found = 0;
    for (u32 i = 0; i + 8 <= size; i++) {
        u32 v2 = read_u32_le(bytes + i);
        u32 v3 = read_u32_le(bytes + i + 4);

        if (v2 == want_v2 && v3 == want_v3) {
            if (found < max_out) {
                out[found].float_offset = i;
                out[found].code_at_minus_24 = (i >= 24) ? read_u32_le(bytes + i - 24) : 0;
                out[found].code_at_minus_8 = (i >= 8) ? read_u32_le(bytes + i - 8) : 0;
                out[found].code_at_plus_0 = read_u32_le(bytes + i);
                out[found].v2_bits = v2;
                out[found].v3_bits = v3;
                out[found].v2 = bits_to_f32(v2);
                out[found].v3 = bits_to_f32(v3);
                if (i >= 24) {
                    u32 base = i - 24;
                    out[found].pre_u32[0] = read_u32_le(bytes + base + 0);
                    out[found].pre_u32[1] = read_u32_le(bytes + base + 4);
                    out[found].pre_u32[2] = read_u32_le(bytes + base + 8);
                    out[found].pre_u32[3] = read_u32_le(bytes + base + 12);
                    out[found].pre_u32[4] = read_u32_le(bytes + base + 16);
                    out[found].pre_u32[5] = read_u32_le(bytes + base + 20);
                }
            }
            found++;
        }
    }

    return found;
}

static void compute_primary_block_metadata(const ClassEntry* entries,
                                           u32 count,
                                           const u8* bytes,
                                           ParseSummary* summary) {
    summary->has_candidate_list_count = 0;
    summary->candidate_list_count_offset = 0;
    summary->candidate_list_count_value = 0;

    if (count == 0) {
        return;
    }

    u32 best_start = 0;
    u32 best_end = 0;
    u32 block_start = 0;

    while (block_start < count) {
        u32 block_end = block_start;
        while (block_end + 1 < count && entries[block_end + 1].float_offset == entries[block_end].float_offset + 88) {
            block_end++;
        }

        if ((block_end - block_start) > (best_end - best_start)) {
            best_start = block_start;
            best_end = block_end;
        }

        block_start = block_end + 1;
    }

    u32 block_count = best_end - best_start + 1;
    u32 first_off = entries[best_start].float_offset;

    for (u32 back = 4; back <= 128; back += 4) {
        if (first_off < back) {
            break;
        }
        u32 candidate_offset = first_off - back;
        u32 candidate = read_u32_le(bytes + candidate_offset);
        if (candidate == block_count) {
            summary->has_candidate_list_count = 1;
            summary->candidate_list_count_offset = candidate_offset;
            summary->candidate_list_count_value = block_count;
            break;
        }
    }
}

static int parse_sendself_file(const char* path, ParseSummary* summary) {
    LoadedFile file = load_file_bytes(path);
    if (!file.bytes || file.size == 0) {
        return 0;
    }

    memset(summary, 0, sizeof(*summary));
    summary->path = path;
    summary->size = file.size;

    u32 payload_offset = 0;
    if (file.size >= 5 && file.bytes[0] == SENDSELF_OPCODE) {
        summary->has_sendself_opcode = 1;
        summary->stream_length_prefix = read_u32_le(file.bytes + 1);
        summary->stream_payload_bytes = file.size - 5;
        payload_offset = 5;
    } else if (file.size >= 4) {
        summary->has_sendself_opcode = 0;
        summary->stream_length_prefix = read_u32_le(file.bytes);
        summary->stream_payload_bytes = file.size - 4;
        payload_offset = 4;
    }

    if (file.size >= payload_offset + 16) {
        summary->guid = read_u64_le(file.bytes + payload_offset);
        summary->character_id = read_u64_le(file.bytes + payload_offset + 8);
    }

    summary->entry_count = scan_class_entries(file.bytes, file.size, summary->entries, 512);
    if (summary->entry_count > 512) {
        summary->entry_count = 512;
    }

    for (u32 i = 0; i < summary->entry_count; i++) {
        if (near_f32(summary->entries[i].v2, 1.70f, 0.001f)
            && near_f32(summary->entries[i].v3, 0.95f, 0.001f)) {
            summary->confidence_170_095++;
        }
    }

    compute_primary_block_metadata(summary->entries, summary->entry_count, file.bytes, summary);
    free_loaded_file(&file);
    return 1;
}

static void print_summary(const ParseSummary* s) {
    printf("== SendSelf BIN Parser ==\n");
    printf("file=%s\n", s->path);
    printf("size=%u\n", s->size);
    printf("opcode=%s\n", s->has_sendself_opcode ? "0x03 (SendSelfToClient)" : "none");
    printf("stream_length_prefix=%u\n", s->stream_length_prefix);
    printf("stream_payload_bytes=%u\n", s->stream_payload_bytes);
    printf("guid_le=0x%016llX\n", (unsigned long long)s->guid);
    printf("character_id_le=0x%016llX\n", (unsigned long long)s->character_id);
    printf("matches=%u\n", s->entry_count);

    for (u32 i = 0; i < s->entry_count; i++) {
        const ClassEntry* e = &s->entries[i];
        u16 code16 = (u16)(e->code_at_minus_24 & 0xFFFF);
        u8 hi = (u8)((code16 >> 8) & 0xFF);
        u8 lo = (u8)(code16 & 0xFF);
        printf("entry[%02u] float_off=%u code[-24]=0x%08X (hi=0x%02X lo=0x%02X) code[-8]=0x%08X code[0]=0x%08X v2=0x%08X (%0.3f) v3=0x%08X (%0.3f)\n",
               i,
               e->float_offset,
               e->code_at_minus_24,
               hi,
               lo,
               e->code_at_minus_8,
               e->code_at_plus_0,
               e->v2_bits,
               e->v2,
               e->v3_bits,
               e->v3);
        printf("          pre_u32=[%08X %08X %08X %08X %08X %08X]\n",
               e->pre_u32[0], e->pre_u32[1], e->pre_u32[2], e->pre_u32[3], e->pre_u32[4], e->pre_u32[5]);
    }

    if (s->entry_count > 0) {
        if (s->has_candidate_list_count) {
            printf("block entries=%u\n", s->candidate_list_count_value);
            printf("candidate_list_count_offset=%u\n", s->candidate_list_count_offset);
        }
        printf("confidence_matches_1p70_0p95=%u/%u\n", s->confidence_170_095, s->entry_count);
    }
}

static int write_csv(const ParseSummary* s, const char* out_path) {
    FILE* f = fopen(out_path, "wb");
    if (!f) {
        return 0;
    }

    fprintf(f, "index,float_offset,code_minus_24,code_minus_8,code_plus_0,v2_bits,v3_bits,v2,v3,pre0,pre1,pre2,pre3,pre4,pre5\n");
    for (u32 i = 0; i < s->entry_count; i++) {
        const ClassEntry* e = &s->entries[i];
        fprintf(f,
                "%u,%u,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,%.6f,%.6f,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X\n",
                i,
                e->float_offset,
                e->code_at_minus_24,
                e->code_at_minus_8,
                e->code_at_plus_0,
                e->v2_bits,
                e->v3_bits,
                e->v2,
                e->v3,
                e->pre_u32[0], e->pre_u32[1], e->pre_u32[2], e->pre_u32[3], e->pre_u32[4], e->pre_u32[5]);
    }

    fclose(f);
    return 1;
}

static int write_json(const ParseSummary* s, const char* out_path) {
    FILE* f = fopen(out_path, "wb");
    if (!f) {
        return 0;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"file\": \"%s\",\n", s->path);
    fprintf(f, "  \"size\": %u,\n", s->size);
    fprintf(f, "  \"hasSendSelfOpcode\": %s,\n", s->has_sendself_opcode ? "true" : "false");
    fprintf(f, "  \"streamLengthPrefix\": %u,\n", s->stream_length_prefix);
    fprintf(f, "  \"streamPayloadBytes\": %u,\n", s->stream_payload_bytes);
    fprintf(f, "  \"guid\": \"0x%016llX\",\n", (unsigned long long)s->guid);
    fprintf(f, "  \"characterId\": \"0x%016llX\",\n", (unsigned long long)s->character_id);
    if (s->has_candidate_list_count) {
        fprintf(f, "  \"candidateListCount\": {\"offset\": %u, \"value\": %u},\n",
                s->candidate_list_count_offset,
                s->candidate_list_count_value);
    } else {
        fprintf(f, "  \"candidateListCount\": null,\n");
    }
    fprintf(f, "  \"confidence170095\": %u,\n", s->confidence_170_095);
    fprintf(f, "  \"entries\": [\n");

    for (u32 i = 0; i < s->entry_count; i++) {
        const ClassEntry* e = &s->entries[i];
        fprintf(f,
                "    {\"index\": %u, \"floatOffset\": %u, \"codeMinus24\": \"0x%08X\", \"codeMinus8\": \"0x%08X\", \"codePlus0\": \"0x%08X\", \"v2Bits\": \"0x%08X\", \"v3Bits\": \"0x%08X\", \"v2\": %.6f, \"v3\": %.6f, \"pre\": [\"0x%08X\", \"0x%08X\", \"0x%08X\", \"0x%08X\", \"0x%08X\", \"0x%08X\"]}%s\n",
                i,
                e->float_offset,
                e->code_at_minus_24,
                e->code_at_minus_8,
                e->code_at_plus_0,
                e->v2_bits,
                e->v3_bits,
                e->v2,
                e->v3,
                e->pre_u32[0], e->pre_u32[1], e->pre_u32[2], e->pre_u32[3], e->pre_u32[4], e->pre_u32[5],
                (i + 1 < s->entry_count) ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
    return 1;
}

static void write_exports(const ParseSummary* s, const char* base_prefix, const char* suffix) {
    char csv_path[512] = { 0 };
    char json_path[512] = { 0 };

    snprintf(csv_path, sizeof(csv_path), "%s%s.csv", base_prefix, suffix ? suffix : "");
    snprintf(json_path, sizeof(json_path), "%s%s.json", base_prefix, suffix ? suffix : "");

    if (write_csv(s, csv_path)) {
        printf("export_csv=%s\n", csv_path);
    } else {
        printf("export_csv_failed=%s\n", csv_path);
    }

    if (write_json(s, json_path)) {
        printf("export_json=%s\n", json_path);
    } else {
        printf("export_json_failed=%s\n", json_path);
    }
}

static void compare_summaries(const ParseSummary* a, const ParseSummary* b) {
    printf("\n== Compare ==\n");
    printf("A=%s\n", a->path);
    printf("B=%s\n", b->path);
    printf("size: %u vs %u\n", a->size, b->size);
    printf("stream_length_prefix: %u vs %u\n", a->stream_length_prefix, b->stream_length_prefix);
    printf("matches: %u vs %u\n", a->entry_count, b->entry_count);

    u32 min_count = a->entry_count < b->entry_count ? a->entry_count : b->entry_count;
    u32 mismatch_count = 0;
    for (u32 i = 0; i < min_count; i++) {
        const ClassEntry* ea = &a->entries[i];
        const ClassEntry* eb = &b->entries[i];
        if (ea->float_offset != eb->float_offset
            || ea->code_at_minus_24 != eb->code_at_minus_24
            || ea->code_at_minus_8 != eb->code_at_minus_8
            || ea->code_at_plus_0 != eb->code_at_plus_0) {
            printf("diff[%u] float_off %u/%u code[-24] 0x%08X/0x%08X code[-8] 0x%08X/0x%08X\n",
                   i,
                   ea->float_offset,
                   eb->float_offset,
                   ea->code_at_minus_24,
                   eb->code_at_minus_24,
                   ea->code_at_minus_8,
                   eb->code_at_minus_8);
            mismatch_count++;
        }
    }
    printf("entry_mismatches=%u\n", mismatch_count);
}

static void print_contiguous_blocks(const ClassEntry* entries, u32 count, const u8* bytes, u32 size) {
    if (count == 0) {
        printf("No class-entry signature blocks found.\n");
        return;
    }

    printf("\n== Contiguous Class-Entry Blocks ==\n");

    u32 block_start = 0;
    while (block_start < count) {
        u32 block_end = block_start;
        while (block_end + 1 < count && entries[block_end + 1].float_offset == entries[block_end].float_offset + 88) {
            block_end++;
        }

        u32 block_count = block_end - block_start + 1;
        u32 first_off = entries[block_start].float_offset;
        u32 last_off = entries[block_end].float_offset;

        printf("block entries=%u first_offset=%u last_offset=%u\n", block_count, first_off, last_off);

        // Candidate list-count often appears right before the first class entry list.
        // We search up to 64 bytes backward for a u32 equal to this block length.
        u32 found_count_offset = 0;
        int has_count_offset = 0;
        for (u32 back = 4; back <= 128; back += 4) {
            if (first_off < back) {
                break;
            }
            u32 candidate_offset = first_off - back;
            u32 candidate = read_u32_le(bytes + candidate_offset);
            if (candidate == block_count) {
                found_count_offset = candidate_offset;
                has_count_offset = 1;
                break;
            }
        }

        if (has_count_offset) {
            printf("  candidate_list_count_offset=%u value=%u\n", found_count_offset, block_count);
        } else {
            printf("  candidate_list_count_offset=not-found-within-128-bytes\n");
        }

        block_start = block_end + 1;
    }
}

int main(int argc, char** argv) {
    const char* file_a = "data/sendself_patched.bin";
    const char* file_b = 0;
    const char* out_prefix = 0;
    int positional_seen = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_prefix = argv[++i];
        } else if (positional_seen == 0) {
            file_a = argv[i];
            positional_seen++;
        } else if (positional_seen == 1) {
            file_b = argv[i];
            positional_seen++;
        }
    }

    ParseSummary a;
    if (!parse_sendself_file(file_a, &a)) {
        fprintf(stderr, "Failed to parse file: %s\n", file_a);
        return 1;
    }

    print_summary(&a);

    if (out_prefix) {
        write_exports(&a, out_prefix, file_b ? "_a" : "");
    }

    if (file_b) {
        ParseSummary b;
        if (!parse_sendself_file(file_b, &b)) {
            fprintf(stderr, "Failed to parse file: %s\n", file_b);
            return 1;
        }

        print_summary(&b);
        compare_summaries(&a, &b);

        if (out_prefix) {
            write_exports(&b, out_prefix, "_b");
        }
    }

    return 0;
}
