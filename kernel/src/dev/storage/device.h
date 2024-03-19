/*
 * kernel/src/dev/storage/device.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/list.h"

struct storage_device;

typedef uint64_t
(*storage_device_read_t)(struct storage_device *device,
                         void *buf,
                         struct range range);

typedef uint64_t
(*storage_device_write_t)(struct storage_device *device,
                          const void *buf,
                          struct range range);

struct storage_device {
    struct list partition_list;

    storage_device_read_t read;
    storage_device_write_t write;

    uint32_t lba_size;
};

bool
storage_device_create(struct storage_device *device,
                      uint32_t lba_size,
                      storage_device_read_t read,
                      storage_device_write_t write);
