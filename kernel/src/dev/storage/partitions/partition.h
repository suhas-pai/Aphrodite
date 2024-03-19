/*
 * kernel/src/dev/storage/partitions/partition.h
 * © suhas pai
 */

#pragma once
#include "dev/storage/device.h"

#define SECTOR_SIZE 512

struct partition {
    struct storage_device *device;
    struct list list;

    struct range lba_range;
};

bool
partition_init(struct partition *partition,
                 struct storage_device *device,
                 const struct range lba_range);

bool
partition_read(const struct partition *partition,
               void *buf,
               struct range range);

bool
partition_write(const struct partition *partition,
                const void *buf,
                struct range range);
