/*
 * lib/adt/addrspace.c
 * © suhas pai
 */

#if defined(BUILD_KERNEL)
    #include "kernel/src/dev/printk.h"
    #include "mm/mm_types.h"
#else
    #define PAGE_SIZE 4096ull
#endif /* defined(BUILD_KERNEL) */

#include "lib/align.h"
#include "addrspace.h"

__optimize(3)
struct addrspace_node *addrspace_node_prev(struct addrspace_node *const node) {
    assert(node->list.prev != NULL);
    if (node->list.prev == &node->addrspace->list) {
        return NULL;
    }

    return container_of(node->list.prev, struct addrspace_node, list);
}

__optimize(3)
struct addrspace_node *addrspace_node_next(struct addrspace_node *const node) {
    if (node->list.prev == &node->addrspace->list) {
        return NULL;
    }

    return container_of(node->list.next, struct addrspace_node, list);
}

enum traversal_result {
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
            prev != NULL ? range_get_end_assert(prev->range) : 0;

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

        if (range_has(hole_range, aligned_range)
            && range_has(in_range, aligned_range))
        {
            *prev_out = prev;
            *result_out = aligned_front;

            return TRAVERSAL_DONE;
        }

        // We fell outside of the free area we found, but we can proceed to the
        // right and find another free area (then from the left).

        if (node->avlnode.right != NULL) {
            struct addrspace_node *const right =
                addrspace_node_of(node->avlnode.right);

            if (right->largest_free_to_prev >= size) {
                *node_out = right;
                return TRAVERSAL_CONTINUE;
            }
        }

        // We didn't find a right node, so we have to go up the tree to a parent
        // node and proceed with the loop from there.

        while (true) {
            if (node->avlnode.parent == NULL) {
                // Since we're at the root, we can only see if there's space to
                // our right.

                struct addrspace_node *const rightmost =
                    container_of(addrspace->list.prev,
                                 struct addrspace_node,
                                 list);

                uint64_t aligned_result =
                    range_get_end_assert(rightmost->range);

                if (aligned_result >= range_get_end_assert(in_range)
                    || !align_up(aligned_result, align, &aligned_result))
                {
                    *result_out = ADDRSPACE_INVALID_ADDR;
                    return TRAVERSAL_DONE;
                }

                uint64_t end = 0;
                if (!check_add(aligned_result, size, &end)) {
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
            node = addrspace_node_of(child->avlnode.parent);

            if (node->avlnode.left == &child->avlnode) {
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
    if (addrspace->avltree.root == NULL) {
        const uint64_t aligned_front =
            align_up_assert(in_range.front, PAGE_SIZE << pagesize_order);

        const struct range aligned_range = RANGE_INIT(aligned_front, size);
        if (!range_has(in_range, aligned_range)) {
            return ADDRSPACE_INVALID_ADDR;
        }

        *prev_out = NULL;
        return aligned_front;
    }

    // Start from the root of the pagemap, and in a loop, proceed to the
    // leftmost node of the address-space to find a free-area. If one isn't
    // found, or isn't acceptable, proceed to the current node's right. If the
    // current node has no right node, go upwards to the parent.

    struct addrspace_node *node = addrspace_node_of(addrspace->avltree.root);
    while (true) {
        // Move to the very left of the address space to find the left-most free
        // area available.

        if (node->avlnode.left != NULL
            && range_is_loc_above(in_range, node->range.front))
        {
            struct addrspace_node *const left =
                addrspace_node_of(node->avlnode.left);

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

__optimize(3) static void avltree_update(struct avlnode *const avlnode) {
    struct addrspace_node *const node = addrspace_node_of(avlnode);
    struct addrspace_node *const prev = addrspace_node_prev(node);

    const uint64_t prev_end =
        prev != NULL ? range_get_end_assert(prev->range) : 0;

    uint64_t largest_free_to_prev = distance(prev_end, node->range.front);
    if (node->avlnode.left != NULL) {
        struct addrspace_node *const left =
            addrspace_node_of(node->avlnode.left);

        largest_free_to_prev =
            max(largest_free_to_prev, left->largest_free_to_prev);
    }

    if (node->avlnode.right != NULL) {
        struct addrspace_node *const right =
            addrspace_node_of(node->avlnode.right);

        largest_free_to_prev =
            max(largest_free_to_prev, right->largest_free_to_prev);
    }

    node->largest_free_to_prev = largest_free_to_prev;
}

__optimize(3) int
avltree_compare(struct avlnode *const our_node,
                struct avlnode *const their_node)
{
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
    struct addrspace_node *prev = NULL;
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

    if (prev != NULL) {
        list_add(&prev->list, &node->list);
        avltree_insert_at_loc(&addrspace->avltree,
                              &node->avlnode,
                              &prev->avlnode,
                              &prev->avlnode.right,
                              avltree_update);
    } else {
        const bool result =
            avltree_insert(&addrspace->avltree,
                           &node->avlnode,
                           avltree_compare,
                           avltree_update,
                           /*added_node=*/NULL);

        assert(result);
        list_add(&addrspace->list, &node->list);
    }

    return addr;
}

__optimize(3) static void add_node_cb(struct avlnode *const avlnode) {
    struct addrspace_node *const node = addrspace_node_of(avlnode);
    struct avlnode *const parent = avlnode->parent;

    if (parent == NULL) {
        list_add(&node->addrspace->list, &node->list);
        return;
    }

    if (avlnode == parent->right) {
        list_add(&addrspace_node_of(parent)->list, &node->list);
    } else {
        struct addrspace_node *const prev =
            addrspace_node_prev(addrspace_node_of(parent));

        assert(prev != NULL);
        list_add(&prev->list, &node->list);
    }
}

__optimize(3) bool
addrspace_add_node(struct address_space *const addrspace,
                   struct addrspace_node *const node)
{
    return
        avltree_insert(&addrspace->avltree,
                       &node->avlnode,
                       avltree_compare,
                       avltree_update,
                       /*added_node=*/add_node_cb);
}

__optimize(3) void addrspace_remove_node(struct addrspace_node *const node) {
    avltree_delete_node(&node->addrspace->avltree,
                        &node->avlnode,
                        avltree_update);

    list_deinit(&node->list);
}

__optimize(3)
void avlnode_print_node_cb(struct avlnode *const avlnode, void *const cb_info) {
    (void)cb_info;
    if (avlnode == NULL) {
        printk(LOGLEVEL_INFO, "(null)");
        return;
    }

    struct addrspace_node *const node = addrspace_node_of(avlnode);
    printk(LOGLEVEL_INFO,
           RANGE_FMT " (largest-gap: 0x%" PRIx64 ")",
           RANGE_FMT_ARGS(node->range),
           node->largest_free_to_prev);
}

#if defined(BUILD_KERNEL)
    __optimize(3)
    void avlnode_print_sv_cb(const struct string_view sv, void *const cb_info) {
        (void)cb_info;
        printk(LOGLEVEL_INFO, SV_FMT, SV_FMT_ARGS(sv));
    }

    __optimize(3) void addrspace_print(struct address_space *const addrspace) {
        avltree_print(&addrspace->avltree,
                      avlnode_print_node_cb,
                      avlnode_print_sv_cb,
                      /*cb_info=*/NULL);
    }
#endif /* defined(BUILD_KERNEL) */