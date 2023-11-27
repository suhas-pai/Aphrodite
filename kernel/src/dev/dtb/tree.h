/*
 * kernel/src/dev/dtb/tree.h
 * © suhas pai
 */

#pragma once
#include "dev/dtb/node.h"

struct devicetree {
    struct devicetree_node *root;
    struct array phandle_list;
};

struct devicetree *devicetree_alloc();
void devicetree_free(struct devicetree *tree);

struct devicetree_node *
devicetree_get_node_for_phandle(struct devicetree *tree, uint32_t phandle);

struct devicetree_node *
devicetree_get_node_at_path(struct devicetree *tree, struct string_view path);