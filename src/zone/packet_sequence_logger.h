#ifndef PACKET_SEQUENCE_LOGGER_H
#define PACKET_SEQUENCE_LOGGER_H

#include <stdio.h>
#include <string.h>
#include <time.h>

// Simple packet sequence tracking for debugging character rendering issues
typedef struct PacketSequenceLog {
    u32 packet_id;
    u32 sequence_number;
    char packet_name[64];
    time_t timestamp;
    b8 was_handled;
} PacketSequenceLog;

#define MAX_PACKET_LOGS 1000
static PacketSequenceLog packet_log[MAX_PACKET_LOGS];
static u32 packet_log_index = 0;

static void log_packet_sequence(u32 packet_id, const char* packet_name, u32 sequence_number, b8 was_handled) {
    if (packet_log_index < MAX_PACKET_LOGS) {
        packet_log[packet_log_index].packet_id = packet_id;
        packet_log[packet_log_index].sequence_number = sequence_number;
        packet_log[packet_log_index].timestamp = time(NULL);
        packet_log[packet_log_index].was_handled = was_handled;
        // Copy packet name safely
        if (packet_name != NULL) {
            strncpy(packet_log[packet_log_index].packet_name, packet_name, sizeof(packet_log[packet_log_index].packet_name) - 1);
            packet_log[packet_log_index].packet_name[sizeof(packet_log[packet_log_index].packet_name) - 1] = '\0';
        }
        packet_log_index++;
    }
}

static void print_packet_sequence_log() {
    printf("\n=== PACKET SEQUENCE LOG ===\n");
    for (u32 i = 0; i < packet_log_index && i < 50; i++) {  // Show last 50 entries
        printf("[%u] Packet: %s (ID: %u, Seq: %u, Handled: %s)\n",
               (u32)packet_log[i].timestamp,
               packet_log[i].packet_name,
               packet_log[i].packet_id,
               packet_log[i].sequence_number,
               packet_log[i].was_handled ? "YES" : "NO");
    }
    printf("=========================\n");
}

#endif // PACKET_SEQUENCE_LOGGER_H