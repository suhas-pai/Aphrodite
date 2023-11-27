/*
 * kernel/src/dev/dtb/driver.h
 * © suhas pai
 */

#pragma once
#include "tree.h"

struct dtb_driver {
    bool (*init)(struct devicetree *tree, struct devicetree_node *node);

    const char *const *compat_list;
    const uint32_t compat_count;
};