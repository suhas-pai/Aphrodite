/*
 * kernel/include/dev/dtb/tree.h
 * © suhas pai
 */

#pragma once
#include <lib/adt/hashmap.h>

#include "dev/dtb/node.h"
#include "mm/simple_alloc.h"

struct devicetree {
    struct devicetree_node *root;

    struct hashmap phandle_map;
    struct simple_alloc alloc;
};

struct devicetree *devicetree_alloc();
struct devicetree *dtb_get_tree();

void
devicetree_init_fields(struct devicetree *tree, struct devicetree_node *root);

void devicetree_free(struct devicetree *tree);

const struct devicetree_node *
devicetree_get_node_for_phandle(const struct devicetree *tree,
                                uint32_t phandle);

struct devicetree_node *
devicetree_get_node_at_path(const struct devicetree *tree,
                            struct string_view path);
