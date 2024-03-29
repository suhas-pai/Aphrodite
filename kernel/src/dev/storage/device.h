/*
 * kernel/src/dev/storage/device.h
 * © suhas pai
 */

#pragma once

#include "dev/storage/cache.h"
#include "lib/adt/range.h"
#include "lib/list.h"

struct storage_device;

typedef uint64_t
(*storage_device_read_t)(struct storage_device *device,
                         uint64_t phys,
                         struct range range);

typedef uint64_t
(*storage_device_write_t)(struct storage_device *device,
                          uint64_t phys,
                          struct range range);

struct storage_device {
    struct list partition_list;
    struct storage_cache cache;

    storage_device_read_t read;
    storage_device_write_t write;

    uint32_t lba_size;
};

bool
storage_device_init(struct storage_device *device,
                    uint32_t lba_size,
                    storage_device_read_t read,
                    storage_device_write_t write);

struct storage_cache;

uint64_t
storage_device_read(struct storage_device *device,
                    void *buf,
                    struct range range);

uint64_t
storage_device_write(struct storage_device *device,
                     const void *buf,
                     struct range range);
