/*
 * lib/adt/addrspace.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/avltree.h"
#include "lib/adt/range.h"

#include "lib/list.h"

struct address_space {
    struct avltree avltree;
    struct list list;
};

struct addrspace_node {
    struct address_space *addrspace;

    struct avlnode avlnode;
    struct list list;
    struct range range;

    // Helper variable used internally to quickly find a free area range.
    // Defined as the maximum of the left subtree, right subtree, and the
    // distance between `range.front` and the previous vm_area's end
    uint64_t largest_free_to_prev;
};

#define ADDRSPACE_INIT(name) \
    ((struct address_space){ \
        .avltree = AVLTREE_INIT(),   \
        .list = LIST_INIT(name.list) \
    })

#define ADDRSPACE_NODE_INIT(name, addrspace_) \
    ((struct addrspace_node){ \
        .addrspace = (addrspace_),     \
        .avlnode = AVLNODE_INIT(),     \
        .list = LIST_INIT(name.list),  \
        .range = RANGE_EMPTY(),        \
        .largest_free_to_prev = 0      \
    })

#define addrspace_node_of(obj) \
    container_of((obj), struct addrspace_node, avlnode)

struct addrspace_node *addrspace_node_prev(struct addrspace_node *node);
struct addrspace_node *addrspace_node_next(struct addrspace_node *node);

#define ADDRSPACE_INVALID_ADDR UINT64_MAX

uint64_t
addrspace_find_space_and_add_node(struct address_space *addrspace,
                                  struct range in_range,
                                  struct addrspace_node *node,
                                  uint64_t align);

bool
addrspace_add_node(struct address_space *addrspace,
                   struct addrspace_node *node);

void addrspace_remove_node(struct addrspace_node *node);
void addrspace_print(struct address_space *addrspace);