/*
 * lib/adt/addrspace.c
 * © suhas pai
 */

#if defined(BUILD_KERNEL)
    #include "dev/printk.h"
    #include "mm/mm_types.h"
#else
    #define PAGE_SIZE 4096ull
#endif /* defined(BUILD_KERNEL) */

#include <lib/align.h>
#include <lib/compare.h>

#include "addrspace.h"

__debug_optimize(3)
struct addrspace_node *addrspace_node_prev(struct addrspace_node *const node) {
    assert(node->list.prev != nullptr);
    if (node->list.prev == &node->addrspace->list) {
        return nullptr;
    }

    return parent_of(node->list.prev, struct addrspace_node, list);
}

__debug_optimize(3)
struct addrspace_node *addrspace_node_next(struct addrspace_node *const node) {
    if (node->list.prev == &node->addrspace->list) {
        return nullptr;
    }

    return parent_of(node->list.next, struct addrspace_node, list);
}

enum traversal_result : uint8_t {
    TRAVERSAL_DONE,
    TRAVERSAL_CONTINUE,
};

static enum traversal_result
traverse_tree(const struct address_space *const addrspace,
              const struct range in_range,
              struct addrspace_node *node,
              const uint64_t size,
              const uint8_t pagesize_order,
              uint64_t *const result_out,
              struct addrspace_node **const node_out,
              struct addrspace_node **const prev_out)
{
    const uint64_t align = PAGE_SIZE << pagesize_order;
    while (true) {
        struct addrspace_node *const prev = addrspace_node_prev(node);
        const uint64_t prev_end =
            prev != nullptr ? range_get_end_assert(prev->range) : 0;

        // prev_end is the lowest possible address we can find a hole at, so if
        // prev_end is above in_range, then we can't find an acceptable free
        // physical range.

        if (range_is_loc_above(in_range, prev_end)) {
            *result_out = ADDRSPACE_INVALID_ADDR;
            return TRAVERSAL_DONE;
        }

        uint64_t aligned_front = 0;
        if (!align_up(prev_end, align, &aligned_front)) {
            *result_out = ADDRSPACE_INVALID_ADDR;
            return TRAVERSAL_DONE;
        }

        const struct range aligned_range = RANGE_INIT(aligned_front, size);
        const struct range hole_range =
            range_create_end(prev_end, node->range.front);

        if (range_has(hole_range, aligned_range) &&
            range_has(in_range, aligned_range))
        {
            *prev_out = prev;
            *result_out = aligned_front;

            return TRAVERSAL_DONE;
        }

        // We fell outside of the free area we found, but we can proceed to the
        // right and find another free area (then from the left).

        if (node->node.right != nullptr) {
            struct addrspace_node *const right =
                addrspace_node_of(node->node.right);

            if (right->largest_free_to_prev >= size) {
                *node_out = right;
                return TRAVERSAL_CONTINUE;
            }
        }

        // We didn't find a right node, so we have to go up the tree to a parent
        // node and proceed with the loop from there.

        while (true) {
            if (node->node.parent == nullptr) {
                // Since we're at the root, we can only see if there's space to
                // our right.

                struct addrspace_node *const rightmost =
                    parent_of(addrspace->list.prev,
                              struct addrspace_node,
                              list);

                uint64_t aligned_result =
                    range_get_end_assert(rightmost->range);

                if (aligned_result >= range_get_end_assert(in_range) ||
                    !align_up(aligned_result, align, &aligned_result))
                {
                    *result_out = ADDRSPACE_INVALID_ADDR;
                    return TRAVERSAL_DONE;
                }

                uint64_t end = 0;
                if (!ckd_add(&end, aligned_result, size)) {
                    *result_out = ADDRSPACE_INVALID_ADDR;
                    return TRAVERSAL_DONE;
                }

                *result_out = aligned_result;
                *prev_out = rightmost;

                return TRAVERSAL_DONE;
            }

            // Keep going up the tree until we find the parent of a left node we
            // were previously at.

            struct addrspace_node *const child = node;
            node = addrspace_node_of(child->node.parent);

            if (node->node.left == &child->node) {
                // We've found the parent of such a left node, break out of this
                // look and go through the entire outside loop again.

                break;
            }
        }
    }
}

static uint64_t
find_from_start(const struct address_space *const addrspace,
                const struct range in_range,
                const uint64_t size,
                const uint8_t pagesize_order,
                struct addrspace_node **const prev_out)
{
#if ADDRSPACE_USE_REDBLACKTREE
    if (redblacktree_empty(&addrspace->tree)) {
#else
    if (avltree_empty(&addrspace->tree)) {
#endif /* ADDRSPACE_USE_REDBLACKTREE */
        const uint64_t aligned_front =
            align_up_assert(in_range.front, PAGE_SIZE << pagesize_order);

        const struct range aligned_range = RANGE_INIT(aligned_front, size);
        if (!range_has(in_range, aligned_range)) {
            return ADDRSPACE_INVALID_ADDR;
        }

        *prev_out = nullptr;
        return aligned_front;
    }

    // Start from the root of the pagemap, and in a loop, proceed to the
    // leftmost node of the address-space to find a free-area. If one isn't
    // found, or isn't acceptable, proceed to the current node's right. If the
    // current node has no right node, go upwards to the parent.

    struct addrspace_node *node = addrspace_node_of(addrspace->tree.root);
    while (true) {
        // Move to the very left of the address space to find the left-most free
        // area available.

        if (node->node.left != nullptr &&
            range_is_loc_above(in_range, node->range.front))
        {
            struct addrspace_node *const left =
                addrspace_node_of(node->node.left);

            if (left->largest_free_to_prev >= size) {
                node = left;
                continue;
            }
        }

        uint64_t result = 0;
        const enum traversal_result traverse_result =
            traverse_tree(addrspace,
                          in_range,
                          node,
                          size,
                          pagesize_order,
                          &result,
                          &node,
                          prev_out);

        switch (traverse_result) {
            case TRAVERSAL_DONE:
                return result;
            case TRAVERSAL_CONTINUE:
                break;
        }
    }
}

#if ADDRSPACE_USE_REDBLACKTREE
    typedef struct redblacktree_node addrspace_tree_node_t;
    typedef struct redblacktree addrspace_tree_t;
#else
    typedef struct avlnode addrspace_tree_node_t;
    typedef struct avltree addrspace_tree_t;
#endif /* ADDRSPACE_USE_REDBLACKTREE */

__debug_optimize(3) static void
update_callback(addrspace_tree_node_t *const tree_node, void *const cb_info) {
    (void)cb_info;

    struct addrspace_node *const node = addrspace_node_of(tree_node);
    struct addrspace_node *const prev = addrspace_node_prev(node);

    const uint64_t prev_end =
        prev != nullptr ? range_get_end_assert(prev->range) : 0;

    uint64_t largest_free_to_prev = distance(prev_end, node->range.front);
    if (node->node.left != nullptr) {
        struct addrspace_node *const left =
            addrspace_node_of(node->node.left);

        largest_free_to_prev =
            max(largest_free_to_prev, left->largest_free_to_prev);
    }

    if (node->node.right != nullptr) {
        struct addrspace_node *const right =
            addrspace_node_of(node->node.right);

        largest_free_to_prev =
            max(largest_free_to_prev, right->largest_free_to_prev);
    }

    node->largest_free_to_prev = largest_free_to_prev;
}

__debug_optimize(3) int
compare_treenodes(addrspace_tree_node_t *const our_node,
                  addrspace_tree_node_t *const their_node,
                  void *const cb_info)
{
    (void)cb_info;

    const struct addrspace_node *const ours = addrspace_node_of(our_node);
    const struct addrspace_node *const theirs = addrspace_node_of(their_node);

    if (range_overlaps(ours->range, theirs->range)) {
        return 0;
    }

    return range_above(ours->range, theirs->range) ? -1 : 1;
}

uint64_t
addrspace_find_space_and_add_node(struct address_space *const addrspace,
                                  const struct range in_range,
                                  struct addrspace_node *const node,
                                  const uint8_t pagesize_order)
{
    struct addrspace_node *prev = nullptr;
    const uint64_t addr =
        find_from_start(addrspace,
                        in_range,
                        node->range.size,
                        pagesize_order,
                        &prev);

    if (addr == ADDRSPACE_INVALID_ADDR) {
        return ADDRSPACE_INVALID_ADDR;
    }

    node->range.front = addr;

    if (prev != nullptr) {
        list_add(&prev->list, &node->list);
    #if ADDRSPACE_USE_REDBLACKTREE
        redblacktree_insert_at_loc(&addrspace->tree,
                                   &node->node,
                                   &prev->node,
                                   &prev->node.right,
                                   update_callback,
                                   /*added_node=*/nullptr,
                                   /*cb_info=*/nullptr);
    #else
        avltree_insert_at_loc(&addrspace->tree,
                              &node->node,
                              &prev->node,
                              &prev->node.right,
                              update_callback,
                              /*added_node=*/nullptr,
                              /*cb_info=*/nullptr);
    #endif /* ADDRSPACE_USE_REDBLACKTREE */
    } else {
    #if ADDRSPACE_USE_REDBLACKTREE
        const bool result =
            redblacktree_insert(&addrspace->tree,
                                &node->node,
                                compare_treenodes,
                                update_callback,
                                /*added_node=*/nullptr,
                                /*cb_info=*/nullptr);
    #else
        const bool result =
            avltree_insert(&addrspace->tree,
                           &node->node,
                           compare_treenodes,
                           update_callback,
                           /*added_node=*/nullptr,
                           /*cb_info=*/nullptr);
    #endif /* ADDRSPACE_USE_REDBLACKTREE */

        assert(result);
        list_add(&addrspace->list, &node->list);
    }

    return addr;
}

__debug_optimize(3) static
void add_node_cb(addrspace_tree_node_t *const tree_node, void *const cb_info) {
    (void)cb_info;

    struct addrspace_node *const node = addrspace_node_of(tree_node);
    addrspace_tree_node_t *const parent = tree_node->parent;

    if (parent == nullptr) {
        list_add(&node->addrspace->list, &node->list);
        return;
    }

    if (tree_node == parent->right) {
        list_add(&addrspace_node_of(parent)->list, &node->list);
    } else {
        struct addrspace_node *const prev =
            addrspace_node_prev(addrspace_node_of(parent));

        assert(prev != nullptr);
        list_add(&prev->list, &node->list);
    }
}

__debug_optimize(3) bool
addrspace_add_node(struct address_space *const addrspace,
                   struct addrspace_node *const node)
{
#if ADDRSPACE_USE_REDBLACKTREE
    return
        redblacktree_insert(&addrspace->tree,
                            &node->node,
                            compare_treenodes,
                            update_callback,
                            /*added_node=*/add_node_cb,
                            /*cb_info=*/nullptr);
#else
    return
        avltree_insert(&addrspace->tree,
                       &node->node,
                       compare_treenodes,
                       update_callback,
                       /*added_node=*/add_node_cb,
                       /*cb_info=*/nullptr);
#endif /* ADDRSPACE_USE_REDBLACKTREE */
}

static int
addrspace_node_range_compare(addrspace_tree_node_t *const treenode,
                             void *const key,
                             void *const cb_info)
{
    (void)cb_info;

    struct addrspace_node *const node = addrspace_node_of(treenode);
    const struct range range = *(struct range *)key;

    return
        range_below(node->range, range) ? LESS_THAN :
        range_above(node->range, range) ? GREATER_THAN : EQUAL_TO;
}

struct addrspace_node *
addrspace_find_node_with_range(struct address_space *const addrspace,
                               struct range range)
{
#if ADDRSPACE_USE_REDBLACKTREE
    addrspace_tree_node_t *const tree_node =
        redblacktree_find(&addrspace->tree,
                          &range,
                          addrspace_node_range_compare,
                          /*cb_info=*/nullptr);
#else
    addrspace_tree_node_t *const tree_node =
        avltree_find(&addrspace->tree,
                     &range,
                     addrspace_node_range_compare,
                     /*cb_info=*/nullptr);
#endif /* ADDRSPACE_USE_REDBLACKTREE */

    if (tree_node == nullptr) {
        return nullptr;
    }

    return addrspace_node_of(tree_node);
}

__debug_optimize(3)
void addrspace_remove_node(struct addrspace_node *const node) {
#if ADDRSPACE_USE_REDBLACKTREE
    redblacktree_delete_node(&node->addrspace->tree,
                             &node->node,
                             update_callback,
                             /*cb_info=*/nullptr);
#else
    avltree_delete_node(&node->addrspace->tree,
                        &node->node,
                        update_callback,
                        /*cb_info=*/nullptr);
#endif /* ADDRSPACE_USE_REDBLACKTREE */

    list_deinit(&node->list);
}

#if defined(BUILD_KERNEL)
__debug_optimize(3)
void
avlnode_print_node_cb(addrspace_tree_node_t *const avlnode,
                      void *const cb_info)
{
    (void)cb_info;
    if (avlnode == nullptr) {
        printk(LOGLEVEL_INFO, "(null)");
        return;
    }

    struct addrspace_node *const node = addrspace_node_of(avlnode);
    printk(LOGLEVEL_INFO,
           RANGE_FMT " (largest-gap: 0x%" PRIx64 ")",
           RANGE_FMT_ARGS(node->range),
           node->largest_free_to_prev);
}
#endif /* defined(BUILD_KERNEL) */

#if defined(BUILD_KERNEL)
    __debug_optimize(3)
    void avlnode_print_sv_cb(const struct string_view sv, void *const cb_info) {
        (void)cb_info;
        printk(LOGLEVEL_INFO, SV_FMT, SV_FMT_ARGS(sv));
    }

    __debug_optimize(3)
    void addrspace_print(struct address_space *const addrspace) {
    #if ADDRSPACE_USE_REDBLACKTREE
        redblacktree_print(&addrspace->tree,
                           avlnode_print_node_cb,
                           avlnode_print_sv_cb,
                           /*cb_info=*/nullptr);
    #else
        avltree_print(&addrspace->tree,
                      avlnode_print_node_cb,
                      avlnode_print_sv_cb,
                      /*cb_info=*/nullptr);
    #endif /* ADDRSPACE_USE_REDBLACKTREE */
    }
#endif /* defined(BUILD_KERNEL) */
