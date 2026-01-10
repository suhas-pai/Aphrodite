/*
 * lib/adt/redblacktree.h
 * © suhas pai
 */

#pragma once
#include <lib/adt/string_view.h>

enum redblacktree_node_color : uint8_t {
    REDBLACKTREE_NODE_COLOR_RED,
    REDBLACKTREE_NODE_COLOR_BLACK,
};

struct redblacktree_node {
    struct redblacktree_node *parent;
    struct redblacktree_node *left;
    struct redblacktree_node *right;

    enum redblacktree_node_color color;
};

bool redblacktree_node_is_right_child(struct redblacktree_node *node);
bool redblacktree_node_is_left_child(struct redblacktree_node *node);
bool redblacktree_node_is_root(struct redblacktree_node *node);

struct redblacktree_node *
redblacktree_node_get_uncle(struct redblacktree_node *node);

struct redblacktree_node *
redblacktree_node_get_sibling(struct redblacktree_node *node);

struct redblacktree_node *
redblacktree_node_get_grandparent(struct redblacktree_node *node);

struct redblacktree_node *
redblacktree_node_leftmost(struct redblacktree_node *node);

struct redblacktree_node *
redblacktree_node_rightmost(struct redblacktree_node *node);

struct redblacktree;

typedef bool
(*redblacktree_traverse_callback_t)(struct redblacktree *tree,
                                    struct redblacktree_node *node,
                                    void *cb_info);

bool
redblacktree_node_preorder(struct redblacktree *tree,
                           struct redblacktree_node *node,
                           redblacktree_traverse_callback_t callback,
                           void *cb_info);
bool
redblacktree_node_inorder(struct redblacktree *tree,
                          struct redblacktree_node *node,
                          redblacktree_traverse_callback_t callback,
                          void *cb_info);

bool
redblacktree_node_postorder(struct redblacktree *tree,
                            struct redblacktree_node *node,
                            redblacktree_traverse_callback_t callback,
                            void *cb_info);

void redblacktree_node_destroy(struct redblacktree_node *node);

struct redblacktree {
    struct redblacktree_node *root;
};

#define REDBLACKTREE_INIT() ((struct redblacktree){ .root = nullptr })
#define REDBLACKTREE_NODE_INIT() \
    ((struct redblacktree_node){ \
        .parent = nullptr, \
        .left = nullptr,   \
        .right = nullptr,  \
        .color = REDBLACKTREE_NODE_COLOR_RED \
    })

typedef int
(*redblacktree_node_compare_t)(struct redblacktree_node *ours,
                               struct redblacktree_node *theirs,
                               void *cb_info);
typedef int
(*redblacktree_node_compare_key_t)(struct redblacktree_node *theirs,
                                   void *key,
                                   void *cb_info);

typedef void
(*redblacktree_node_print_node_cb_t)(struct redblacktree_node *node,
                                     void *cb_info);
typedef void
(*redblacktree_node_print_sv_cb_t)(struct string_view sv, void *cb_info);

bool redblacktree_empty(const struct redblacktree *tree);

void
redblacktree_node_print(struct redblacktree_node *node,
                        redblacktree_node_print_node_cb_t print_node_cb,
                        redblacktree_node_print_sv_cb_t print_sv_cb,
                        void *cb_info);

// Callback called when node is moved in tree
typedef void
(*redblacktree_node_moved_t)(struct redblacktree_node *node,
                             void *cb_info);

typedef void
(*redblacktree_node_added_node_t)(struct redblacktree_node *node,
                                  void *cb_info);

void
redblacktree_node_merge(struct redblacktree_node *left,
                        struct redblacktree_node *right);

bool
redblacktree_insert(struct redblacktree *tree,
                    struct redblacktree_node *node,
                    redblacktree_node_compare_t comparator,
                    redblacktree_node_moved_t moved,
                    redblacktree_node_added_node_t added_node,
                    void *cb_info);

void
redblacktree_insert_at_loc(struct redblacktree *tree,
                           struct redblacktree_node *node,
                           struct redblacktree_node *parent,
                           struct redblacktree_node **link,
                           redblacktree_node_moved_t moved,
                           redblacktree_node_added_node_t added_node,
                           void *cb_info);

struct redblacktree_node *
redblacktree_find(struct redblacktree *tree,
                  void *key,
                  redblacktree_node_compare_key_t comparator,
                  void *cb_info);

struct redblacktree_node *
redblacktree_delete(struct redblacktree *tree,
                    void *key,
                    redblacktree_node_compare_key_t comparator,
                    redblacktree_node_moved_t moved,
                    void *cb_info);

void
redblacktree_delete_node(struct redblacktree *tree,
                         struct redblacktree_node *node,
                         redblacktree_node_moved_t moved,
                         void *cb_info);

struct redblacktree_node *
redblacktree_leftmost(const struct redblacktree *tree);

struct redblacktree_node *
redblacktree_rightmost(const struct redblacktree *tree);

void
redblacktree_print(struct redblacktree *tree,
                   redblacktree_node_print_node_cb_t print_node_cb,
                   redblacktree_node_print_sv_cb_t print_sv_cb,
                   void *cb_info);
