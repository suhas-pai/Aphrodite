/*
 * kernel/sr/dev/storage/gpt.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct gpt_header {
    char signature[8];

    uint32_t revision;
    uint32_t length;
    uint32_t crc32;

    const uint32_t reserved;

    uint64_t header_lba;
    uint64_t alt_header_lba;

    uint64_t first_usable_lba;
    uint64_t last_usable_lba;

    uint64_t guid_low64;
    uint64_t guid_hi64;

    uint64_t partition_sector;

    uint32_t partition_entry_count;
    uint32_t partition_entry_size;
    uint32_t partition_entry_crc32;
} __packed;

#define GPT_HEADER_MIN_SIZE 92
#define GPT_HEADER_MAGIC_SV SV_STATIC("EFI PART")

// First lba
#define GPT_HEADER_LOCATION SECTOR_SIZE

enum gpt_entry_attr_flags {
    __GPT_ENTRY_ATTR_USED_BY_EFI = 1 << 0,
    __GPT_ENTRY_ATTR_REQUIRED_TO_FUNCTION = 1 << 1,
    __GPT_ENTRY_ATTR_USED_BY_OS = 1 << 2,
    __GPT_ENTRY_ATTR_REQUIRED_BY_OS = 1 << 3,
    __GPT_ENTRY_ATTR_BACKUP_REQUIRED = 1 << 4,
    __GPT_ENTRY_ATTR_USER_DATA = 1 << 5,
    __GPT_ENTRY_ATTR_CRITICAL_USER_DATA = 1 << 6,
    __GPT_ENTRY_ATTR_REDUNDANT_PARTITION = 1 << 7,
};

struct gpt_entry {
    uint64_t typelow;
    uint64_t typehi;

    uint64_t guid_low64;
    uint64_t guid_hi64;

    uint64_t start;
    uint64_t end;

    uint64_t attributes;
    uint16_t name[36]; // In UTF-16
};

bool verify_gpt_header(const struct gpt_header *header);
bool gpt_entry_mount_ok(const struct gpt_entry *entry);
