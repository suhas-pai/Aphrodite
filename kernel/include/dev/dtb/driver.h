/*
 * kernel/src/dev/dtb/driver.h
 * © suhas pai
 */

#pragma once
#include "tree.h"

enum dtb_driver_match_flags {
    __DTB_DRIVER_MATCH_COMPAT = 1 << 0,
    __DTB_DRIVER_MATCH_DEVICE_TYPE = 1 << 1,
};

struct dtb_driver {
    bool
    (*init)(const struct devicetree *tree, const struct devicetree_node *node);

    uint32_t match_flags;

    const struct string_view *const compat_list;
    const uint32_t compat_count;

    const struct string_view device_type;
};
