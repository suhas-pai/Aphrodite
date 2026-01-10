/*
 * lib/adt/avltree.c
 * © suhas pai
 */

#include <lib/compare.h>
#include "avltree.h"

__debug_optimize(3) static inline
void avlnode_verify(struct avlnode *const node, struct avlnode *const parent) {
#if defined (BUILD_TEST)
    if (node == nullptr) {
        return;
    }

    assert(node != node->left);
    assert(node != node->right);
    assert(node->parent == parent);

    if (node->parent != nullptr) {
        assert(node->parent->left == node || node->parent->right == node);
    }

    avlnode_verify(node->left, node);
    avlnode_verify(node->right, node);
#else
    (void)node;
    (void)parent;
#endif /* defined(BUILD_TEST ) */
}

__debug_optimize(3) static struct avlnode *
go_up_parents(struct avlnode *node, uint32_t *const depth_level_in) {
    struct avlnode *parent = node->parent;
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
parent_has_next(struct avlnode *node,
                const uint32_t node_depth_level,
                const uint32_t depth_index)
{
    struct avlnode *parent = node->parent;
    const uint32_t count = node_depth_level - depth_index - 1;

    for_upto_limit(count, i) {
        node = parent;
        parent = parent->parent;
    }

    return parent->left == node;
}

__debug_optimize(3) static void
print_prefix_lines(struct avlnode *const current,
                   const avlnode_print_sv_cb_t print_sv_cb,
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

bool
avltree_node_preorder(struct avltree *const tree,
                      struct avlnode *const node,
                      const avltree_traverse_callback_t callback,
                      void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!callback(tree, node, cb_info)) {
        return false;
    }

    return (
        avltree_node_preorder(tree, node->left, callback, cb_info) ||
        avltree_node_preorder(tree, node->right, callback, cb_info)
    );
}

bool
avltree_node_inorder(struct avltree *const tree,
                     struct avlnode *const node,
                     const avltree_traverse_callback_t callback,
                     void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!avltree_node_inorder(tree, node->left, callback, cb_info)) {
        return false;
    }

    if (!callback(tree, node, cb_info)) {
        return false;
    }

    return avltree_node_inorder(tree, node->right, callback, cb_info);
}

bool
avltree_node_postorder(struct avltree *const tree,
                       struct avlnode *const node,
                       const avltree_traverse_callback_t callback,
                       void *const cb_info)
{
    if (node == nullptr) {
        return true;
    }

    if (!avltree_node_postorder(tree, node->left, callback, cb_info)) {
        return false;
    }

    if (!avltree_node_postorder(tree, node->right, callback, cb_info)) {
        return false;
    }

    return callback(tree, node, cb_info);
}

__debug_optimize(3) bool avltree_empty(const struct avltree *const tree) {
    return tree->root == nullptr;
}

void
avlnode_print(struct avlnode *const node,
              const avlnode_print_node_cb_t print_node_cb,
              const avlnode_print_sv_cb_t print_sv_cb,
              void *const cb_info)
{
    if (node == nullptr) {
        return;
    }

    print_node_cb(node, cb_info);

    struct avlnode *current = node;
    struct avlnode *parent = current->parent;

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

__debug_optimize(3) static inline void
avlnode_update(struct avlnode *const node,
               const avltree_node_moved_t moved,
               void *const cb_info)
{
    if (moved != nullptr) {
        moved(node, cb_info);
    }
}

__debug_optimize(3) static struct avlnode *
rotate_left(struct avlnode *const node,
            const avltree_node_moved_t moved,
            void *const cb_info)
{
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->right;
    struct avlnode *const new_top_left = node;
    struct avlnode *const new_top_parent = node->parent;
    struct avlnode *const new_top_left_right = node->right->left;

    avlnode_verify(new_top, new_top->parent);

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
    }

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    avlnode_update(node, moved, cb_info);
    avlnode_update(new_top, moved, cb_info);

    return new_top;
}

__debug_optimize(3) static struct avlnode *
rotate_right(struct avlnode *const node,
             const avltree_node_moved_t moved,
             void *const cb_info)
{
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->left;
    struct avlnode *const new_top_right = node;
    struct avlnode *const new_top_parent = node->parent;
    struct avlnode *const new_top_right_left = node->left->right;

    avlnode_verify(new_top, new_top->parent);

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
    }

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    avlnode_update(node, moved, cb_info);
    avlnode_update(new_top, moved, cb_info);

    return new_top;
}

__debug_optimize(3)
static inline uint32_t node_height(const struct avlnode *const node) {
    return node != nullptr ? node->height : 0;
}

__debug_optimize(3)
static inline int64_t node_balance(const struct avlnode *const node) {
    return (int64_t)node_height(node->right) - node_height(node->left);
}

__debug_optimize(3)
static inline void reset_node_height(struct avlnode *const node) {
    node->height = 1 + max(node_height(node->left), node_height(node->right));
}

__debug_optimize(3) static inline struct avlnode **
get_node_link(struct avlnode *const node, struct avltree *const tree) {
    struct avlnode *const parent = node->parent;
    if (parent != nullptr) {
        return (node == parent->left) ? &parent->left : &parent->right;
    }

    return &tree->root;
}

static void
avltree_fixup(struct avltree *const tree,
              struct avlnode *node,
              const avltree_node_moved_t moved,
              void *const cb_info)
{
    while (true) {
        if (node == nullptr) {
            return;
        }

        struct avlnode *new_node = nullptr;
        const int64_t balance = node_balance(node);

        if (balance > 1) {
            // Right-heavy

            new_node = node->right;
            const bool double_rotate =
                node_height(new_node->left) > node_height(new_node->right);

            if (double_rotate) {
                // We need to perform a double rotation - Right-Left rotation
                new_node = rotate_right(new_node, moved, cb_info);
                node->right = new_node;

                reset_node_height(new_node);
                avlnode_update(new_node, moved, cb_info);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_left(node, moved, cb_info);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            node = new_node;
        } else if (balance < -1) {
            // Left-Heavy

            new_node = node->left;
            const bool double_rotate =
                node_height(new_node->left) < node_height(new_node->right);

            if (double_rotate) {
                // We need to perform a double rotation - Left-Right rotation
                new_node = rotate_left(new_node, moved, cb_info);
                node->left = new_node;

                reset_node_height(new_node);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_right(node, moved, cb_info);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            node = new_node;
        } else {
            reset_node_height(node);
            avlnode_update(node, moved, cb_info);
        }

        avlnode_verify(node, node->parent);
        node = node->parent;
    }
}

__debug_optimize(3) bool
avltree_insert(struct avltree *const tree,
               struct avlnode *const node,
               const avlnode_compare_t comparator,
               const avltree_node_moved_t node_moved,
               const avltree_node_added_t added_node,
               void *const cb_info)
{
    struct avlnode *parent = nullptr;
    struct avlnode *curr_node = tree->root;
    struct avlnode **link = &tree->root;

    while (curr_node != nullptr) {
        const int compare = comparator(node, curr_node, cb_info);
        if (compare == 0) {
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

    avltree_insert_at_loc(tree,
                          node,
                          parent,
                          link,
                          node_moved,
                          added_node,
                          cb_info);
    return true;
}

__debug_optimize(3) void
avltree_insert_at_loc(struct avltree *const tree,
                      struct avlnode *const node,
                      struct avlnode *const parent,
                      struct avlnode **const link,
                      const avltree_node_moved_t moved,
                      const avltree_node_added_t added_node,
                      void *const cb_info)
{
    node->height = 1;
    node->left = nullptr;
    node->right = nullptr;
    node->parent = parent;

    *link = node;
    avlnode_verify(node, parent);

    if (added_node != nullptr) {
        added_node(node, cb_info);
    }

    avlnode_update(node, moved, cb_info);
    avltree_fixup(tree, parent, moved, cb_info);
}

struct avlnode *
avltree_find(struct avltree *const tree,
             void *const key,
             const avlnode_compare_key_t comparator,
             void *const cb_info)
{
    struct avlnode *curr_node = tree->root;
    while (curr_node != nullptr) {
        const int compare = comparator(curr_node, key, cb_info);
        if (compare == 0) {
            return curr_node;
        }

        curr_node = is_less_than(compare) ? curr_node->left : curr_node->right;
    }

    return nullptr;
}

__debug_optimize(3) struct avlnode *
avltree_delete(struct avltree *const tree,
               void *const key,
               const avlnode_compare_key_t comparator,
               const avltree_node_moved_t moved,
               void *const cb_info)
{
    struct avlnode *curr_node = tree->root;
    while (curr_node != nullptr) {
        const int compare = comparator(curr_node, key, cb_info);
        if (compare == 0) {
            avltree_delete_node(tree, curr_node, moved, cb_info);
            return curr_node;
        }

        curr_node = compare < 0 ? curr_node->left : curr_node->right;
    }

    return nullptr;
}

__debug_optimize(3) struct avlnode *avlnode_successor(struct avlnode *node) {
    if (node->right != nullptr) {
        node = node->right;
        while (node->left != nullptr) {
            node = node->left;
        }

        return node;
    }

    node = node->parent;
    while (node != nullptr) {
        if (node->left != nullptr) {
            return node;
        }

        node = node->parent;
    }

    return nullptr;
}

void
avltree_delete_node(struct avltree *const tree,
                    struct avlnode *const node,
                    const avltree_node_moved_t moved,
                    void *const cb_info)
{
    struct avlnode **node_link = get_node_link(node, tree);
    struct avlnode *parent = node->parent;

    if (node->left != nullptr) {
        if (node->right != nullptr) {
            struct avlnode *successor = node->right;
            struct avlnode *const left = node->left;

            if (successor->left != nullptr) {
                struct avlnode **successor_link = &node->right;
                do {
                    parent = successor;
                    successor_link = &successor->left;
                    successor = successor->left;
                } while (successor->left != nullptr);

                successor->left = left;
                left->parent = successor;
                node->left = nullptr;

                swap(node->right, successor->right);
                successor->right->parent = successor;

                if (node->right != nullptr) {
                    node->right->parent = node;
                }

                *successor_link = node;
                *node_link = successor;

                successor->parent = node->parent;
                node->parent = parent;

                swap(node->height, successor->height);
                node_link = successor_link;
            } else {
                node->left = nullptr;
                node->right = successor->right;

                successor->left = left;
                successor->right = node;

                left->parent = successor;
                if (node->right != nullptr) {
                    node->right->parent = node;
                }

                successor->right = node;
                successor->parent = node->parent;

                node->parent = successor;
                *node_link = successor;

                swap(node->height, successor->height);

                node_link = &successor->right;
                parent = successor;
            }
        }
    }

    struct avlnode *const first_child = node->left ?: node->right;
    *node_link = first_child;

    if (first_child != nullptr) {
        first_child->parent = parent;
    }

    avltree_fixup(tree, parent, moved, cb_info);
}

__debug_optimize(3)
struct avlnode *avltree_leftmost(const struct avltree *const tree) {
    struct avlnode *node = tree->root;
    if (node == nullptr) {
        return nullptr;
    }

    while (true) {
        if (node->left == nullptr) {
            return node;
        }

        node = node->left;
    }
}

__debug_optimize(3)
struct avlnode *avltree_rightmost(const struct avltree *const tree) {
    struct avlnode *node = tree->root;
    if (node == nullptr) {
        return nullptr;
    }

    while (true) {
        if (node->right == nullptr) {
            return node;
        }

        node = node->right;
    }
}

__debug_optimize(3) void
avltree_print(struct avltree *const tree,
              const avlnode_print_node_cb_t print_node_cb,
              const avlnode_print_sv_cb_t print_sv_cb,
              void *const cb_info)
{
    avlnode_print(tree->root, print_node_cb, print_sv_cb, cb_info);
}
