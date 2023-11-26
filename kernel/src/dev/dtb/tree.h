/*
 * dev/dtb/tree.h
 * © suhas pai
 */

#pragma once
#include "dev/dtb/node.h"

struct devicetree {
    struct devicetree_node *root;
    struct array phandle_list;
};

struct devicetree *devicetree_alloc();