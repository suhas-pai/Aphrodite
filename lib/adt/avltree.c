/*
 * lib/adt/avltree.c
 * © suhas pai
 */

#include <stdbool.h>

#include "lib/assert.h"
#include "avltree.h"

__optimize(3) static inline
void avlnode_verify(struct avlnode *const node, struct avlnode *const parent) {
#if defined (BUILD_TEST)
    if (node == NULL) {
        return;
    }

    assert(node != node->left);
    assert(node != node->right);
    assert(node->parent == parent);

    avlnode_verify(node->left, node);
    avlnode_verify(node->right, node);
#else
    (void)node;
    (void)parent;
#endif /* defined(BUILD_TEST ) */
}

__optimize(3) static struct avlnode *
go_up_parents(struct avlnode *node, uint32_t *const depth_level_in) {
    struct avlnode *parent = node->parent;
    uint32_t depth_level = *depth_level_in;

    while (parent != NULL) {
        if (parent->left == node) {
            *depth_level_in = depth_level;
            return parent->right;
        }

        depth_level--;

        node = parent;
        parent = parent->parent;
    }

    *depth_level_in = depth_level;
    return NULL;
}

__optimize(3) static bool
parent_has_next(struct avlnode *node,
                const uint32_t node_depth_level,
                const uint32_t depth_index)
{
    struct avlnode *parent = node->parent;
    const uint32_t count = node_depth_level - depth_index - 1;

    for (uint32_t i = 0; i != count; i++) {
        node = parent;
        parent = parent->parent;
    }

    return parent->left == node;
}

__optimize(3) static void
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

void
avlnode_print(struct avlnode *const node,
              const avlnode_print_node_cb_t print_node_cb,
              const avlnode_print_sv_cb_t print_sv_cb,
              void *const cb_info)
{
    if (node == NULL) {
        return;
    }

    print_node_cb(node, cb_info);

    struct avlnode *current = node;
    struct avlnode *parent = current->parent;

    uint32_t depth_level = 1;
    do {
        print_sv_cb(SV_STATIC("\n"), cb_info);
        if (current->left == NULL) {
            if (current->right == NULL) {
                current = go_up_parents(current, &depth_level);
                if (current == NULL) {
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

        if (parent->left == NULL) {
            print_prefix_lines(current, print_sv_cb, cb_info, depth_level);

            print_sv_cb(SV_STATIC("├── "), cb_info);
            print_node_cb(NULL, cb_info);
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

        if (parent->right == NULL) {
            print_sv_cb(SV_STATIC("\n"), cb_info);
            print_prefix_lines(current, print_sv_cb, cb_info, depth_level);

            print_sv_cb(SV_STATIC("└── "), cb_info);
            print_node_cb(NULL, cb_info);
        }
    } while (true);
}

__optimize(3) static struct avlnode *rotate_left(struct avlnode *const node) {
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->right;
    struct avlnode *const new_top_left = node;
    struct avlnode *const new_top_parent = node->parent;
    struct avlnode *const new_top_left_right = node->right->left;

    avlnode_verify(new_top, new_top->parent);

    new_top_left->right = new_top_left_right;
    new_top_left->parent = new_top;

    if (new_top_left_right != NULL) {
        new_top_left_right->parent = new_top_left;
    }

    new_top->parent = new_top_parent;
    new_top->left = new_top_left;

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    return new_top;
}

__optimize(3) static struct avlnode *rotate_right(struct avlnode *const node) {
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->left;
    struct avlnode *const new_top_right = node;
    struct avlnode *const new_top_parent = node->parent;
    struct avlnode *const new_top_right_left = node->left->right;

    avlnode_verify(new_top, new_top->parent);

    new_top_right->left = new_top_right_left;
    new_top_right->parent = new_top;

    if (new_top_right_left != NULL) {
        new_top_right_left->parent = new_top_right;
    }

    new_top->parent = new_top_parent;
    new_top->right = new_top_right;

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    return new_top;
}

__optimize(3)
static inline uint32_t node_height(const struct avlnode *const node) {
    return node != NULL ? node->height : 0;
}

__optimize(3)
static inline int64_t node_balance(const struct avlnode *const node) {
    return (int64_t)node_height(node->right) - node_height(node->left);
}

__optimize(3) static inline void reset_node_height(struct avlnode *const node) {
    node->height = 1 + max(node_height(node->left), node_height(node->right));
}

__optimize(3) static inline struct avlnode **
get_node_link(struct avlnode *const node, struct avltree *const tree) {
    struct avlnode *const parent = node->parent;
    if (parent != NULL) {
        return (node == parent->left) ? &parent->left : &parent->right;
    }

    return &tree->root;
}

__optimize(3) static inline
void avlnode_update(struct avlnode *const node, const avlnode_update_t update) {
    if (update != NULL) {
        update(node);
    }
}

static void
avltree_fixup(struct avltree *const tree,
              struct avlnode *node,
              const avlnode_update_t update)
{
    while (true) {
        if (node == NULL) {
            return;
        }

        struct avlnode *new_node = NULL;
        const int64_t balance = node_balance(node);

        if (balance > 1) {
            // Right-heavy

            new_node = node->right;
            const bool double_rotate =
                node_height(new_node->left) > node_height(new_node->right);

            if (double_rotate) {
                // We need to perform a double rotation - Right-Left rotation
                new_node = rotate_right(new_node);
                node->right = new_node;

                reset_node_height(new_node);
                avlnode_update(new_node, update);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_left(node);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            avlnode_update(node, update);

            node = new_node;
        } else if (balance < -1) {
            // Left-Heavy

            new_node = node->left;
            const bool double_rotate =
                node_height(new_node->left) < node_height(new_node->right);

            if (double_rotate) {
                // We need to perform a double rotation - Left-Right rotation
                new_node = rotate_left(new_node);
                node->left = new_node;

                reset_node_height(new_node);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_right(node);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            avlnode_update(node, update);

            node = new_node;
        } else {
            reset_node_height(node);
        }

        avlnode_verify(node, node->parent);
        avlnode_update(node, update);

        node = node->parent;
    }
}

__optimize(3) static void
insert_at_loc(struct avltree *const tree,
              struct avlnode *const node,
              struct avlnode *const parent,
              struct avlnode **const link,
              const avlnode_update_t update,
              const avlnode_added_node_t added_node)
{
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;

    *link = node;
    avlnode_verify(node, parent);

    if (added_node != NULL) {
        added_node(node);
    }

    avlnode_update(node, update);
    avltree_fixup(tree, parent, update);
}

__optimize(3) bool
avltree_insert(struct avltree *const tree,
               struct avlnode *const node,
               const avlnode_compare_t comparator,
               const avlnode_update_t update,
               const avlnode_added_node_t added_node)
{
    struct avlnode *parent = NULL;
    struct avlnode *curr_node = tree->root;
    struct avlnode **link = &tree->root;

    while (curr_node != NULL) {
        const int compare = comparator(node, curr_node);
        if (compare == 0) {
            return false;
        }

        parent = curr_node;
        if (compare < 0) {
            link = &curr_node->left;
            curr_node = curr_node->left;
        } else {
            link = &curr_node->right;
            curr_node = curr_node->right;
        }
    }

    insert_at_loc(tree, node, parent, link, update, added_node);
    return true;
}

__optimize(3) void
avltree_insert_at_loc(struct avltree *const tree,
                      struct avlnode *const node,
                      struct avlnode *const parent,
                      struct avlnode **const link,
                      const avlnode_update_t update)
{
    insert_at_loc(tree, node, parent, link, update, /*added_node=*/NULL);
}

__optimize(3) void
avltree_delete(struct avltree *const tree,
               void *const key,
               const avlnode_compare_key_t comparator,
               const avlnode_update_t update)
{
    struct avlnode *curr_node = tree->root;
    while (curr_node != NULL) {
        const int compare = comparator(curr_node, key);
        if (compare == 0) {
            avltree_delete_node(tree, curr_node, update);
            return;
        }

        curr_node = compare < 0 ? curr_node->left : curr_node->right;
    }
}

__optimize(3) struct avlnode *avlnode_successor(struct avlnode *node) {
    if (node->right != NULL) {
        node = node->right;
        while (node->left != NULL) {
            node = node->left;
        }

        return node;
    }

    node = node->parent;
    while (node != NULL) {
        if (node->left != NULL) {
            return node;
        }

        node = node->parent;
    }

    return NULL;
}

void
avltree_delete_node(struct avltree *const tree,
                    struct avlnode *const node,
                    const avlnode_update_t update)
{
    struct avlnode **node_link = get_node_link(node, tree);
    struct avlnode *parent = node->parent;

    if (node->left != NULL) {
        if (node->right != NULL) {
            struct avlnode *successor = node->right;
            struct avlnode *const left = node->left;

            if (successor->left != NULL) {
                struct avlnode **successor_link = &node->right;
                do {
                    parent = successor;
                    successor_link = &successor->left;
                    successor = successor->left;
                } while (successor->left != NULL);

                successor->left = left;
                left->parent = successor;
                node->left = NULL;

                swap(node->right, successor->right);
                successor->right->parent = successor;

                if (node->right != NULL) {
                    node->right->parent = node;
                }

                *successor_link = node;
                *node_link = successor;

                successor->parent = node->parent;
                node->parent = parent;

                swap(node->height, successor->height);
                node_link = successor_link;
            } else {
                node->left = NULL;
                node->right = successor->right;

                successor->left = left;
                successor->right = node;

                left->parent = successor;
                if (node->right != NULL) {
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

    if (first_child != NULL) {
        first_child->parent = parent;
    }

    avltree_fixup(tree, parent, update);
}

__optimize(3)
struct avlnode *avltree_leftmost(const struct avltree *const tree) {
    struct avlnode *node = tree->root;
    if (node == NULL) {
        return NULL;
    }

    while (true) {
        if (node->left == NULL) {
            return node;
        }

        node = node->left;
    }
}

__optimize(3)
struct avlnode *avltree_rightmost(const struct avltree *const tree) {
    struct avlnode *node = tree->root;
    if (node == NULL) {
        return NULL;
    }

    while (true) {
        if (node->right == NULL) {
            return node;
        }

        node = node->right;
    }
}

__optimize(3) void
avltree_print(struct avltree *const tree,
              const avlnode_print_node_cb_t print_node_cb,
              const avlnode_print_sv_cb_t print_sv_cb,
              void *const cb_info)
{
    avlnode_print(tree->root, print_node_cb, print_sv_cb, cb_info);
}