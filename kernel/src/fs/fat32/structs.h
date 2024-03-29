/*
 * kernel/src/fs/fat32/structs.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

#define FAT32_BOOTRECORD_SIGNATURE 0x29
#define FAT32_BOOTRECORD_IDENTIFIER "FAT32   "

struct fat32_bootrecord {
    uint8_t jmp[3];
    uint8_t oem_id[8];

    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t fat_count;
    uint16_t root_direntries;
    uint16_t sector_count;
    uint16_t sectors_per_fat_unused;
    uint8_t media_desc;
    uint32_t geometry;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t rootdir_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint32_t reserved[3];
    uint8_t drive;
    uint8_t flagsnt;
    uint8_t signature;
    uint32_t volume_id;

    char volume_label[11];
    char identifier[8];
} __packed;
