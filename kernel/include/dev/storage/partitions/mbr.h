/*
 * kernel/src/dev/storage/mbr.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct mbr_entry {
    uint8_t boot_indicator;

    uint8_t start[3];
    uint8_t os_type;
    uint8_t end[3];

    uint32_t starting_sector;
    uint32_t sector_count;
};

struct mbr_header {
    uint16_t magic;
    struct mbr_entry entry_list[4];
} __packed;

#define MBR_HEADER_MAGIC 0x5a5a
#define MBR_HEADER_LOCATION 444

bool verify_mbr_header(const struct mbr_header *header);
bool verify_mbr_entry(const struct mbr_entry *entry);
