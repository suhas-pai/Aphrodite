/*
 * tests/avltree.c
 * © suhas pai
 */

#include <inttypes.h>
#include <stdio.h>

#include "lib/adt/avltree.h"
#include "lib/list.h"

struct node {
    struct avlnode info;
    uint32_t number;
};

static int compare(struct node *const ours, struct node *const theirs) {
    return (int64_t)ours->number - theirs->number;
}

static int identify(struct node *const theirs, const void *const key) {
    return (int64_t)key - (int64_t)theirs->number;
}

static void insert_node(struct avltree *const tree, const uint32_t number) {
    struct node *const avl_node = malloc(sizeof(struct node));
    avl_node->number = number;

    const bool result =
        avltree_insert(tree,
                       (struct avlnode *)avl_node,
                       (avlnode_compare_t)compare,
                       /*update=*/NULL,
                       /*added_node=*/NULL);

    assert(result);
}

void avlnode_print_node_cb(struct avlnode *const avlnode, void *const cb_info) {
    (void)cb_info;
    if (avlnode == NULL) {
        printf("(null)");
        return;
    }

    struct node *const node = container_of(avlnode, struct node, info);

    printf("%" PRIu32, node->number);
    fflush(stdout);
}

void avlnode_print_sv_cb(const struct string_view sv, void *const cb_info) {
    (void)cb_info;

    printf(SV_FMT, SV_FMT_ARGS(sv));
    fflush(stdout);
}

static void print_tree(struct avltree *const tree) {
    avltree_print(tree, avlnode_print_node_cb, avlnode_print_sv_cb, NULL);
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

    print_tree(&tree);

    avltree_delete(&tree, (void *)53, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)11, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)21, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)9, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)8, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)61, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)33, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)73, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)71, (avlnode_compare_key_t)identify, NULL);

    printf("After deleting:\n");
    print_tree(&tree);
}