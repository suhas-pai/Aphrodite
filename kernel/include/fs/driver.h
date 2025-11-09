/*
 * kernel/include/fs/driver.h
 * © suhas pai
 */

#pragma once
#include <lib/adt/string_view.h>

struct partition;
struct fs_driver {
    struct string_view name;
    bool (*try_init)(struct partition *partition);
} __aligned(16);

extern char fs_drivers_start[];
extern char fs_drivers_end[];

#define fs_driver_foreach(iter) \
    ptrrange_foreach((struct fs_driver *)(uint64_t)fs_drivers_start, \
                     (struct fs_driver *)(uint64_t)fs_drivers_end, \
                     iter)

#define __fs_driver __attribute__((used, section(".fs_drivers")))
