/*
 * kernel/src/fs/driver.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/string_view.h"

struct partition;
struct fs_driver {
    const struct string_view name;
    bool (*try_init)(struct partition *partition);
} __aligned(16);

extern char fs_drivers_start[];
extern char fs_drivers_end[];

#define fs_driver_foreach(iter) \
    for (struct fs_driver *iter = \
            (struct fs_driver *)(uint64_t)fs_drivers_start; \
         iter < (struct fs_driver *)(uint64_t)fs_drivers_end; \
         iter++) \

#define __fs_driver __attribute__((used, section(".fs_drivers")))
