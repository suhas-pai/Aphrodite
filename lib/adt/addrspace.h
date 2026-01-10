/*
 * lib/adt/addrspace.h
 * © suhas pai
 */

#pragma once

#include <lib/adt/avltree.h>
#include <lib/adt/redblacktree.h>
#include <lib/adt/range.h>

#include <lib/list.h>

#define ADDRSPACE_USE_REDBLACKTREE 1

struct address_space {
#if ADDRSPACE_USE_REDBLACKTREE
    struct redblacktree tree;
#else
    struct avltree tree;
#endif /* ADDRSPACE_USE_REDBLACKTREE */

    struct list list;
};

struct addrspace_node {
    struct address_space *addrspace;

#if ADDRSPACE_USE_REDBLACKTREE
    struct redblacktree_node node;
#else
    struct avlnode node;
#endif /* ADDRSPACE_USE_REDBLACKTREE */

    struct list list;
    struct range range;

    // Helper variable used internally to quickly find a free area range.
    // Defined as the maximum of the left subtree, right subtree, and the
    // distance between `range.front` and the previous vm_area's end
    uint64_t largest_free_to_prev;
};

#if ADDRSPACE_USE_REDBLACKTREE
    #define ADDRSPACE_INIT(lvalue) \
        ((struct address_space){ \
            .tree = REDBLACKTREE_INIT(), \
            .list = LIST_INIT(lvalue.list) \
        })

    #define ADDRSPACE_NODE_INIT(lvalue, addrspace_) \
        ((struct addrspace_node){ \
            .addrspace = (addrspace_), \
            .node = REDBLACKTREE_NODE_INIT(), \
            .list = LIST_INIT(lvalue.list),  \
            .range = RANGE_EMPTY(), \
            .largest_free_to_prev = 0 \
        })
#else
    #define ADDRSPACE_INIT(lvalue) \
        ((struct address_space){ \
            .tree = AVLTREE_INIT(), \
            .list = LIST_INIT(lvalue.list) \
        })

    #define ADDRSPACE_NODE_INIT(lvalue, addrspace_) \
        ((struct addrspace_node){ \
            .addrspace = (addrspace_), \
            .node = AVLNODE_INIT(), \
            .list = LIST_INIT(lvalue.list),  \
            .range = RANGE_EMPTY(), \
            .largest_free_to_prev = 0 \
        })
#endif /* ADDRSPACE_USE_REDBLACKTREE */

#define addrspace_node_of(obj) parent_of((obj), struct addrspace_node, node)
#define addrspace_foreach_node(addrspace, node) \
    list_foreach(&addrspace->list, list, node)

struct addrspace_node *addrspace_node_prev(struct addrspace_node *node);
struct addrspace_node *addrspace_node_next(struct addrspace_node *node);

#define ADDRSPACE_INVALID_ADDR UINT64_MAX

uint64_t
addrspace_find_space_and_add_node(struct address_space *addrspace,
                                  struct range in_range,
                                  struct addrspace_node *node,
                                  uint8_t pagesize_order);

bool
addrspace_add_node(struct address_space *addrspace,
                   struct addrspace_node *node);

struct addrspace_node *
addrspace_find_node_with_range(struct address_space *addrspace,
                               struct range range);

void addrspace_remove_node(struct addrspace_node *node);
void addrspace_print(struct address_space *addrspace);
