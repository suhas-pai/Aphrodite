/*
 * lib/adt/avltree.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/string_view.h"

struct avlnode {
    struct avlnode *parent;
    struct avlnode *left;
    struct avlnode *right;

    uint32_t height;
};

typedef void (*avlnode_print_node_cb_t)(struct avlnode *node, void *cb_info);
typedef void (*avlnode_print_sv_cb_t)(struct string_view sv, void *cb_info);

void
avlnode_print(struct avlnode *node,
              avlnode_print_node_cb_t print_node_cb,
              avlnode_print_sv_cb_t print_sv_cb,
              void *cb_info);

struct avltree {
    struct avlnode *root;
};

#define AVLTREE_INIT() ((struct avltree){ .root = NULL })
#define AVLNODE_INIT() \
    ((struct avlnode){ \
        .parent = NULL, \
        .left = NULL,   \
        .right = NULL,  \
        .height = 0     \
    })

typedef int (*avlnode_compare_t)(struct avlnode *ours, struct avlnode *theirs);
typedef int (*avlnode_compare_key_t)(struct avlnode *theirs, void *key);

typedef void (*avlnode_update_t)(struct avlnode *node);
typedef void (*avlnode_added_node_t)(struct avlnode *node);

void avlnode_merge(struct avlnode *left, struct avlnode *right);

bool
avltree_insert(struct avltree *tree,
               struct avlnode *node,
               avlnode_compare_t comparator,
               avlnode_update_t update,
               avlnode_added_node_t added_node);

void
avltree_insert_at_loc(struct avltree *tree,
                      struct avlnode *node,
                      struct avlnode *parent,
                      struct avlnode **link,
                      avlnode_update_t update);

void
avltree_delete(struct avltree *tree,
               void *key,
               avlnode_compare_key_t identifier,
               avlnode_update_t update);

void
avltree_delete_node(struct avltree *tree,
                    struct avlnode *node,
                    avlnode_update_t update);

struct avlnode *avltree_leftmost(const struct avltree *tree);
struct avlnode *avltree_rightmost(const struct avltree *tree);

typedef struct range
(*avlnode_get_range_t)(struct avltree *tree,
                       struct avlnode *node,
                       void *cb_info);

struct range
avltree_find_free_space(struct avltree *tree,
                        struct avlnode *leftmost,
                        avlnode_get_range_t get_range_cb,
                        void *cb_info);

void
avltree_print(struct avltree *tree,
              avlnode_print_node_cb_t print_node_cb,
              avlnode_print_sv_cb_t print_sv_cb,
              void *cb_info);