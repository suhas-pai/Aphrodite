/*
 * kernel/include/dev/storage/partitions/partition.h
 * © suhas pai
 */

#pragma once

#include <lib/adt/string.h>
#include "dev/storage/device.h"

#define SECTOR_SIZE 512

struct partition {
    struct storage_device *device;

    struct list list;
    struct string name;

    struct range range;
};

void
partition_init(struct partition *partition,
               struct string name,
               struct storage_device *device,
               const struct range lba_range);

uint64_t
partition_read(const struct partition *partition,
               struct range range,
               void *buf);

uint64_t
partition_write(const struct partition *partition,
                struct range range,
                const void *buf);
