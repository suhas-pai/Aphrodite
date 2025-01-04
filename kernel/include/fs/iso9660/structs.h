/*
 * kernel/src/fs/iso9660/structs.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct iso9660_primary_volume_desc {
    uint8_t type;

    char magic[5];
    uint8_t version;

    char system_id[32];
    char volume_id[32];

    uint32_t volume_space_size;
};
