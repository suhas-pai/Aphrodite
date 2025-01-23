/*
 * kernel/include/dev/dtb/bus.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "dev/device.h"

struct dtb_device {
    struct device device;

    const struct devicetree *tree;
    const struct devicetree_node *node;
};
