/*
 * tests/avltree.c
 * © suhas pai
 */

#include <inttypes.h>
#include <stdio.h>

#include <lib/adt/avltree.h>

struct node {
    struct avlnode info;
    uint32_t number;
};

static int
compare(struct avlnode *const ours,
        struct avlnode *const theirs,
        void *const cb_info)
{
    struct node *const our_node = parent_of(ours, struct node, info);
    struct node *const their_node = parent_of(theirs, struct node, info);

    return (int64_t)our_node->number - their_node->number;
}

static int
identify(struct avlnode *const theirs, void *const key, void *const cb_info) {
    struct node *const their_node = parent_of(theirs, struct node, info);
    return (int64_t)key - (int64_t)their_node->number;
}

static void insert_node(struct avltree *const tree, const uint32_t number) {
    struct node *const avl_node = malloc(sizeof(struct node));
    avl_node->number = number;

    const bool result =
        avltree_insert(tree,
                       (struct avlnode *)avl_node,
                       compare,
                       /*update=*/nullptr,
                       /*added_node=*/nullptr,
                       /*cb_info=*/nullptr);

    assert(result);
}

void avlnode_print_node_cb(struct avlnode *const avlnode, void *const cb_info) {
    (void)cb_info;
    if (avlnode == nullptr) {
        printf("(null)");
        return;
    }

    struct node *const node = parent_of(avlnode, struct node, info);

    printf("%" PRIu32, node->number);
    fflush(stdout);
}

void avlnode_print_sv_cb(const struct string_view sv, void *const cb_info) {
    (void)cb_info;

    printf(SV_FMT, SV_FMT_ARGS(sv));
    fflush(stdout);
}

static void print_tree(struct avltree *const tree) {
    avltree_print(tree, avlnode_print_node_cb, avlnode_print_sv_cb, nullptr);
}

void test_avltree() {
    struct avltree tree = AVLTREE_INIT();

    insert_node(&tree, 8);
    insert_node(&tree, 9);
    insert_node(&tree, 11);
    insert_node(&tree, 21);
    insert_node(&tree, 33);
    insert_node(&tree, 53);
    insert_node(&tree, 61);
    insert_node(&tree, 73);
    insert_node(&tree, 71);

    assert(avltree_find(&tree, (void *)33, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)73, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)9, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)21, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)53, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)61, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)71, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)8, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)11, identify, nullptr) != nullptr);
    assert(avltree_find(&tree, (void *)5, identify, nullptr) == nullptr);

    // print_tree(&tree);
    free(avltree_delete(&tree, (void *)53, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)11, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)21, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)9, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)8, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)61, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)33, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)73, identify, nullptr, nullptr));
    free(avltree_delete(&tree, (void *)71, identify, nullptr, nullptr));

    assert(tree.root == nullptr);
    printf("avltree: All tests passed!\n");
}