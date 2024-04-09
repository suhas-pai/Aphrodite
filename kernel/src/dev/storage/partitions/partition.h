/*
 * kernel/src/dev/storage/partitions/partition.h
 * © suhas pai
 */

#pragma once

#include "dev/storage/device.h"
#include "lib/adt/string.h"

#define SECTOR_SIZE 512

struct partition {
    struct storage_device *device;

    struct list list;
    struct string name;

    struct range range;
};

bool
partition_init(struct partition *partition,
               struct string name,
               struct storage_device *device,
               const struct range lba_range);

uint64_t
partition_read(const struct partition *partition,
               void *buf,
               struct range range);

uint64_t
partition_write(const struct partition *partition,
                const void *buf,
                struct range range);
