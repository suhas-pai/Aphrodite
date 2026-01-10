/*
 * lib/adt/redblacktree.c
 * © suhas pai
 */

#include <lib/compare.h>
#include "redblacktree.h"

__debug_optimize(3) static inline
bool redblacktree_node_is_red(struct redblacktree_node *const node) {
    return node != nullptr && node->color == REDBLACKTREE_NODE_COLOR_RED;
}

__debug_optimize(3) static inline
bool redblacktree_node_is_black(struct redblacktree_node *const node) {
    return node != nullptr && node->color == REDBLACKTREE_NODE_COLOR_BLACK;
}

__debug_optimize(3) static inline void
redblacktree_node_verify(struct redblacktree *const tree,
                         struct redblacktree_node *const node,
                         struct redblacktree_node *const parent)
{
    if (node == nullptr) {
        return;
    }

    assert(node != node->left);
    assert(node != node->right);
    assert(node->parent == parent);

    switch (node->color) {
        case REDBLACKTREE_NODE_COLOR_RED: {
            // Red nodes cannot have red children

            struct redblacktree_node *const left = node->left;
            struct redblacktree_node *const right = node->right;

            if (left != nullptr) {
                assert(left->color == REDBLACKTREE_NODE_COLOR_BLACK);
            }

            if (right != nullptr) {
                assert(right->color == REDBLACKTREE_NODE_COLOR_BLACK);
            }

            break;
        }
        case REDBLACKTREE_NODE_COLOR_BLACK: {
            // Every path from a node to its descendant null nodes (leaves) has
            // the same number of black nodes.

            // TODO
            break;
        }
        default:
            verify_not_reached();
    }

    if (redblacktree_node_is_red(node)) {
        assert(node->left == nullptr || redblacktree_node_is_black(node->left));
        assert(node->right == nullptr ||
               redblacktree_node_is_black(node->right));
    }

    if (node->parent != nullptr) {
        assert(node->parent->left == node || node->parent->right == node);
    } else {
        assert(tree->root == node);
        assert(node->color == REDBLACKTREE_NODE_COLOR_BLACK);
    }

    redblacktree_node_verify(tree, node->left, node);
    redblacktree_node_verify(tree, node->right, node);
}

static inline void redblacktree_verify(struct redblacktree *const tree) {
    redblacktree_node_verify(tree, tree->root, nullptr);
}

__debug_optimize(3)
bool redblacktree_node_is_right_child(struct redblacktree_node *const node) {
    struct redblacktree_node *const parent = node->parent;
    if (parent != nullptr) {
        return parent->right == node;
    }

    return false;
}

__debug_optimize(3)
bool redblacktree_node_is_left_child(struct redblacktree_node *const node) {
    struct redblacktree_node *const parent = node->parent;
    if (parent != nullptr) {
        return parent->left == node;
    }

    return false;
}

__debug_optimize(3)
bool redblacktree_node_is_root(struct redblacktree_node *const node) {
    return node->parent == nullptr;
}

__debug_optimize(3) struct redblacktree_node *
redblacktree_node_get_uncle(struct redblacktree_node *const node) {
    struct redblacktree_node *const parent = node->parent;
    if (parent != nullptr) {
        return redblacktree_node_get_sibling(parent);
    }

    return nullptr;
}

__debug_optimize(3) struct redblacktree_node *
redblacktree_node_get_sibling(struct redblacktree_node *const node) {
    struct redblacktree_node *const parent = node->parent;
    if (parent != nullptr) {
        return parent->left == node ? parent->right : parent->left;
    }

    return nullptr;
}

__debug_optimize(3) struct redblacktree_node *
redblacktree_node_get_grandparent(struct redblacktree_node *const node) {
    struct redblacktree_node *const parent = node->parent;
    if (parent != nullptr) {
        return parent->parent;
    }

    return nullptr;
}

struct redblacktree_node *
redblacktree_node_leftmost(struct redblacktree_node *const node) {
    if (node == nullptr) {
        return nullptr;
    }

    // Find the leftmost
    struct redblacktree_node *curr_node = node;
    while (curr_node->left != nullptr) {
        curr_node = curr_node->left;
    }

    return curr_node;
}

struct redblacktree_node *
redblacktree_node_rightmost(struct redblacktree_node *node) {
    if (node == nullptr) {
        return nullptr;
    }

    // Find the leftmost
    struct redblacktree_node *curr_node = node;
    while (curr_node->right != nullptr) {
        curr_node = curr_node->right;
    }

    return curr_node;
}

bool
redblacktree_node_preorder(struct redblacktree *const tree,
                           struct redblacktree_node *const node,
                           const redblacktree_traverse_callback_t callback,
                           void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!callback(tree, node, cb_info)) {
        return false;
    }

    return (
        redblacktree_node_preorder(tree, node->left, callback, cb_info) ||
        redblacktree_node_preorder(tree, node->right, callback, cb_info)
    );
}

bool
redblacktree_node_inorder(struct redblacktree *const tree,
                          struct redblacktree_node *const node,
                          const redblacktree_traverse_callback_t callback,
                          void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!redblacktree_node_inorder(tree, node->left, callback, cb_info)) {
        return false;
    }

    if (!callback(tree, node, cb_info)) {
        return false;
    }

    return redblacktree_node_inorder(tree, node->right, callback, cb_info);
}

bool
redblacktree_node_postorder(struct redblacktree *const tree,
                            struct redblacktree_node *const node,
                            const redblacktree_traverse_callback_t callback,
                            void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!redblacktree_node_postorder(tree, node->left, callback, cb_info)) {
        return false;
    }

    if (!redblacktree_node_postorder(tree, node->right, callback, cb_info)) {
        return false;
    }

    return callback(tree, node, cb_info);
}

__debug_optimize(3) static inline void
redblacktree_node_added_node(struct redblacktree_node *const node,
                             redblacktree_node_added_node_t const added_node,
                             void *const cb_info)
{
    if (added_node != nullptr) {
        added_node(node, cb_info);
    }
}

__debug_optimize(3) static inline void
redblacktree_node_moved(struct redblacktree_node *const node,
                        const redblacktree_node_moved_t moved,
                        void *const cb_info)
{
    if (moved != nullptr) {
        moved(node, cb_info);
    }
}

bool
redblacktree_insert(struct redblacktree *const tree,
                    struct redblacktree_node *const node,
                    redblacktree_node_compare_t const comparator,
                    const redblacktree_node_moved_t moved,
                    redblacktree_node_added_node_t const added_node,
                    void *const cb_info)
{
    struct redblacktree_node *parent = nullptr;
    struct redblacktree_node *curr_node = tree->root;
    struct redblacktree_node **link = &tree->root;

    while (curr_node != nullptr) {
        const int compare = comparator(node, curr_node, cb_info);
        if (is_equal(compare)) {
            return false;
        }

        parent = curr_node;
        if (is_less_than(compare)) {
            link = &curr_node->left;
            curr_node = curr_node->left;
        } else {
            link = &curr_node->right;
            curr_node = curr_node->right;
        }
    }

    redblacktree_insert_at_loc(tree,
                               node,
                               parent,
                               link,
                               moved,
                               added_node,
                               cb_info);
    return true;
}

static void
redblacktree_rotate_left(struct redblacktree *const tree,
                         struct redblacktree_node *const node,
                         const redblacktree_node_moved_t moved,
                         void *const cb_info)
{
    struct redblacktree_node *const new_top = node->right;
    struct redblacktree_node *const new_top_left = node;
    struct redblacktree_node *const new_top_parent = node->parent;
    struct redblacktree_node *const new_top_left_right = node->right->left;

    new_top_left->right = new_top_left_right;
    new_top_left->parent = new_top;

    if (new_top_left_right != nullptr) {
        new_top_left_right->parent = new_top_left;
    }

    new_top->parent = new_top_parent;
    new_top->left = new_top_left;

    if (new_top_parent != nullptr) {
        if (new_top_parent->left == node) {
            new_top_parent->left = new_top;
        } else {
            new_top_parent->right = new_top;
        }
    } else {
        tree->root = new_top;
    }

    redblacktree_node_moved(new_top, moved, cb_info);
    redblacktree_node_moved(node, moved, cb_info);
}

static void
redblacktree_rotate_right(struct redblacktree *const tree,
                          struct redblacktree_node *const node,
                          const redblacktree_node_moved_t moved,
                          void *const cb_info)
{
    struct redblacktree_node *const new_top = node->left;
    struct redblacktree_node *const new_top_right = node;
    struct redblacktree_node *const new_top_parent = node->parent;
    struct redblacktree_node *const new_top_right_left = node->left->right;

    new_top_right->left = new_top_right_left;
    new_top_right->parent = new_top;

    if (new_top_right_left != nullptr) {
        new_top_right_left->parent = new_top_right;
    }

    new_top->parent = new_top_parent;
    new_top->right = new_top_right;

    if (new_top_parent != nullptr) {
        if (new_top_parent->left == node) {
            new_top_parent->left = new_top;
        } else {
            new_top_parent->right = new_top;
        }
    } else {
        tree->root = new_top;
    }

    redblacktree_node_moved(new_top, moved, cb_info);
    redblacktree_node_moved(node, moved, cb_info);
}

__debug_optimize(3) static inline
void redblacktree_node_set_black(struct redblacktree_node *const node) {
    if (node != nullptr) {
        node->color = REDBLACKTREE_NODE_COLOR_BLACK;
    }
}

__debug_optimize(3) static inline
void redblacktree_node_set_red(struct redblacktree_node *const node) {
    if (node != nullptr) {
        node->color = REDBLACKTREE_NODE_COLOR_RED;
    }
}

static void
redblacktree_insert_fixup(struct redblacktree *const tree,
                          struct redblacktree_node *const parent,
                          struct redblacktree_node *const node,
                          redblacktree_node_moved_t const moved,
                          void *const cb_info)
{
    struct redblacktree_node *curr_node = node;
    struct redblacktree_node *curr_parent = parent;

    while (redblacktree_node_is_red(curr_parent)) {
        struct redblacktree_node *grandparent = curr_parent->parent;
        if (grandparent->right == curr_parent) {
            if (redblacktree_node_is_red(grandparent->left)) {
                redblacktree_node_set_black(grandparent->left);
                redblacktree_node_set_black(curr_parent);
                redblacktree_node_set_red(grandparent);

                curr_node = grandparent;
            } else {
                if (redblacktree_node_is_left_child(curr_node)) {
                    curr_node = curr_parent;
                    redblacktree_rotate_right(tree, curr_node, moved, cb_info);

                    curr_parent = curr_node->parent;
                    grandparent = curr_parent->parent;
                }

                redblacktree_node_set_black(curr_parent);
                redblacktree_node_set_red(grandparent);

                redblacktree_rotate_left(tree, grandparent, moved, cb_info);
            }
        } else {
            if (redblacktree_node_is_red(grandparent->right)) {
                redblacktree_node_set_black(grandparent->right);
                redblacktree_node_set_black(curr_parent);
                redblacktree_node_set_red(grandparent);

                curr_node = grandparent;
            } else {
                if (redblacktree_node_is_right_child(curr_node)) {
                    curr_node = curr_parent;
                    redblacktree_rotate_left(tree,
                                             curr_parent,
                                             moved,
                                             cb_info);

                    curr_parent = curr_node->parent;
                    grandparent = curr_parent->parent;
                }

                redblacktree_node_set_black(curr_parent);
                redblacktree_node_set_red(grandparent);

                redblacktree_rotate_right(tree, grandparent, moved, cb_info);
            }
        }

        if (curr_node == tree->root) {
            break;
        }

        curr_parent = curr_node->parent;
        grandparent = curr_parent->parent;
    }

    tree->root->color = REDBLACKTREE_NODE_COLOR_BLACK;
}

void
redblacktree_insert_at_loc(struct redblacktree *const tree,
                           struct redblacktree_node *const node,
                           struct redblacktree_node *const parent,
                           struct redblacktree_node **const link,
                           const redblacktree_node_moved_t moved,
                           const redblacktree_node_added_node_t added_node,
                           void *const cb_info)
{
    redblacktree_verify(tree);

    node->color = REDBLACKTREE_NODE_COLOR_RED;
    node->parent = parent;
    node->left = nullptr;
    node->right = nullptr;

    *link = node;

    redblacktree_node_added_node(node, added_node, cb_info);
    redblacktree_node_moved(node, moved, cb_info);

    if (parent != nullptr && parent->parent == nullptr) {
        return;
    }

    redblacktree_insert_fixup(tree, parent, node, moved, cb_info);
    redblacktree_verify(tree);
}

struct redblacktree_node *
redblacktree_find(struct redblacktree *const tree,
                  void *const key,
                  const redblacktree_node_compare_key_t comparator,
                  void *const cb_info)
{
    struct redblacktree_node *curr_node = tree->root;
    while (curr_node) {
        int cmp = comparator(curr_node, key, cb_info);
        if (is_less_than(cmp)) {
            curr_node = curr_node->left;
        } else if (is_greater_than(cmp)) {
            curr_node = curr_node->right;
        } else {
            return curr_node;
        }
    }

    return nullptr;
}

void
redblacktree_node_transplant(struct redblacktree *const tree,
                             struct redblacktree_node *const left,
                             struct redblacktree_node *const right)
{
    if (left == nullptr) {
        return;
    }

    if (left->parent == nullptr) {
        tree->root = right;
    } else if (left == left->parent->left) {
        left->parent->left = right;
    } else {
        left->parent->right = right;
    }

    if (right != nullptr) {
        right->parent = left->parent;
    }
}

static void
redblacktree_delete_fixup(struct redblacktree *const tree,
                          struct redblacktree_node *x,
                          redblacktree_node_moved_t moved,
                          void *const cb_info)
{
    while (x != tree->root && redblacktree_node_is_black(x)) {
        struct redblacktree_node *w = redblacktree_node_get_sibling(x);
        if (redblacktree_node_is_left_child(x)) {
            if (redblacktree_node_is_red(w)) {
                redblacktree_node_set_black(w);
                redblacktree_node_set_red(x->parent);
                redblacktree_rotate_left(tree, x->parent, moved, cb_info);

                w = x->parent->right;
            }

            if (redblacktree_node_is_black(w->left) &&
                redblacktree_node_is_black(w->right))
            {
                redblacktree_node_set_red(w);
                x = x->parent;
            } else {
                if (redblacktree_node_is_black(w->right)) {
                    redblacktree_node_set_black(w->left);
                    redblacktree_node_set_red(w);
                    redblacktree_rotate_right(tree, w, moved, cb_info);

                    w = x->parent->right;
                }

                w->color = x->parent->color;

                redblacktree_node_set_black(x->parent);
                redblacktree_node_set_black(w->right);

                redblacktree_rotate_left(tree, x->parent, moved, cb_info);
                x = tree->root;
            }
        } else {
            if (redblacktree_node_is_red(w)) {
                redblacktree_node_set_black(w);
                redblacktree_node_set_red(x->parent);
                redblacktree_rotate_right(tree, x->parent, moved, cb_info);

                w = x->parent->left;
            }

            if (redblacktree_node_is_black(w->left) &&
                redblacktree_node_is_black(w->right))
            {
                redblacktree_node_set_red(w);
                x = x->parent;
            } else {
                if (redblacktree_node_is_black(w->left)) {
                    redblacktree_node_set_black(w->right);
                    redblacktree_node_set_red(w);
                    redblacktree_rotate_left(tree, w, moved, cb_info);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                redblacktree_node_set_black(x->parent);
                redblacktree_node_set_black(w->left);
                redblacktree_rotate_right(tree, x->parent, moved, cb_info);
                x = tree->root;
            }
        }
    }

    redblacktree_node_set_black(x);
}

void
redblacktree_delete_node(struct redblacktree *const tree,
                         struct redblacktree_node *const node,
                         const redblacktree_node_moved_t moved,
                         void *const cb_info)
{
    redblacktree_verify(tree);

    enum redblacktree_node_color orig_color = node->color;
    struct redblacktree_node *x = nullptr;

    if (node->left == nullptr) {
        x = node->right;
        redblacktree_node_transplant(tree, node, node->right);
    } else if (node->right == nullptr) {
        x = node->left;
        redblacktree_node_transplant(tree, node, node->left);
    } else {
        struct redblacktree_node *y = redblacktree_node_leftmost(node->right);

        orig_color = y->color;
        x = y->right;

        if (y->parent == node) {
            x->parent = y;
        } else {
            redblacktree_node_transplant(tree, y, y->right);

            y->right = node->right;
            y->right->parent = y;
        }

        redblacktree_node_transplant(tree, node, y);

        y->left = node->left;
        y->left->parent = y;
        y->color = node->color;
    }

    if (orig_color == REDBLACKTREE_NODE_COLOR_BLACK) {
        redblacktree_delete_fixup(tree, x, moved, cb_info);
    }

    redblacktree_verify(tree);
    redblacktree_node_moved(node, moved, cb_info);
}

struct redblacktree_node *
redblacktree_delete(struct redblacktree *tree,
                    void *key,
                    redblacktree_node_compare_key_t comparator,
                    redblacktree_node_moved_t moved,
                    void *cb_info)
{
    struct redblacktree_node *curr_node = tree->root;
    while (curr_node) {
        int cmp = comparator(curr_node, key, cb_info);
        if (is_less_than(cmp)) {
            curr_node = curr_node->left;
        } else if (is_greater_than(cmp)) {
            curr_node = curr_node->right;
        } else {
            redblacktree_delete_node(tree, curr_node, moved, cb_info);
            return curr_node;
        }
    }

    return nullptr;
}

__debug_optimize(3) static struct redblacktree_node *
go_up_parents(struct redblacktree_node *node, uint32_t *const depth_level_in) {
    struct redblacktree_node *parent = node->parent;
    uint32_t depth_level = *depth_level_in;

    while (parent != nullptr) {
        if (parent->left == node) {
            *depth_level_in = depth_level;
            return parent->right;
        }

        depth_level--;

        node = parent;
        parent = parent->parent;
    }

    *depth_level_in = depth_level;
    return nullptr;
}

__debug_optimize(3) static bool
parent_has_next(struct redblacktree_node *node,
                const uint32_t node_depth_level,
                const uint32_t depth_index)
{
    struct redblacktree_node *parent = node->parent;
    const uint32_t count = node_depth_level - depth_index - 1;

    for_upto_limit(count, i) {
        node = parent;
        parent = parent->parent;
    }

    return parent->left == node;
}

__debug_optimize(3) static void
print_prefix_lines(struct redblacktree_node *const current,
                   const redblacktree_node_print_sv_cb_t print_sv_cb,
                   void *const cb_info,
                   const uint32_t depth_level)
{
    const struct string_view spaces = SV_STATIC("    ");
    for (uint32_t i = 1; i < depth_level - 1; i++) {
        if (parent_has_next(current, depth_level, i)) {
            print_sv_cb(SV_STATIC("│"), cb_info);
            print_sv_cb(sv_drop_front(spaces), cb_info);
        } else {
            print_sv_cb(spaces, cb_info);
        }
    }
}

__debug_optimize(3)
bool redblacktree_empty(const struct redblacktree *const tree) {
    return tree->root == nullptr;
}

void
redblacktree_node_print(struct redblacktree_node *const node,
                        const redblacktree_node_print_node_cb_t print_node_cb,
                        const redblacktree_node_print_sv_cb_t print_sv_cb,
                        void *const cb_info)
{
    if (node == nullptr) {
        return;
    }

    print_node_cb(node, cb_info);

    struct redblacktree_node *current = node;
    struct redblacktree_node *parent = current->parent;

    uint32_t depth_level = 1;
    do {
        print_sv_cb(SV_STATIC("\n"), cb_info);
        if (current->left == nullptr) {
            if (current->right == nullptr) {
                current = go_up_parents(current, &depth_level);
                if (current == nullptr) {
                    return;
                }

                parent = current->parent;
            } else {
                parent = current;
                current = parent->right;

                depth_level++;
            }
        } else {
            parent = current;
            current = parent->left;

            depth_level++;
        }

        if (parent->left == nullptr) {
            print_prefix_lines(current, print_sv_cb, cb_info, depth_level);

            print_sv_cb(SV_STATIC("├── "), cb_info);
            print_node_cb(nullptr, cb_info);
            print_sv_cb(SV_STATIC("\n"), cb_info);
        }

        print_prefix_lines(current, print_sv_cb, cb_info, depth_level);
        if (parent->left == current) {
            print_sv_cb(SV_STATIC("├"), cb_info);
        } else {
            print_sv_cb(SV_STATIC("└"), cb_info);
        }

        print_sv_cb(SV_STATIC("── "), cb_info);
        print_node_cb(current, cb_info);

        if (parent->right == nullptr) {
            print_sv_cb(SV_STATIC("\n"), cb_info);
            print_prefix_lines(current, print_sv_cb, cb_info, depth_level);

            print_sv_cb(SV_STATIC("└── "), cb_info);
            print_node_cb(nullptr, cb_info);
        }
    } while (true);
}
