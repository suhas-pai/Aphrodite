/*
 * kernel/src/dev/dtb/tree.h
 * © suhas pai
 */

#pragma once
#include "dev/dtb/node.h"
#include "lib/adt/hashmap.h"

struct devicetree {
    struct devicetree_node *root;
    struct hashmap phandle_map;
};

struct devicetree *devicetree_alloc();

void
devicetree_init_fields(struct devicetree *tree, struct devicetree_node *root);

void devicetree_free(struct devicetree *tree);

const struct devicetree_node *
devicetree_get_node_for_phandle(const struct devicetree *tree,
                                uint32_t phandle);

struct devicetree_node *
devicetree_get_node_at_path(const struct devicetree *tree,
                            struct string_view path);