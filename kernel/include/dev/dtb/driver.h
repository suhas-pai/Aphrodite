/*
 * kernel/include/dev/dtb/driver.h
 * © suhas pai
 */

#pragma once
#include "dev/driver.h"

enum dtb_driver_match_flags : uint8_t {
    __DTB_DRIVER_MATCH_COMPAT = 1 << 0,
    __DTB_DRIVER_MATCH_DEVICE_TYPE = 1 << 1,
};

struct dtb_driver {
    const struct string_view *const compat_list;
    const uint32_t compat_count;

    const struct string_view device_type;
    uint32_t match_flags;

    struct driver driver;
};
