/*
 * tests/avltree.c
 * © suhas pai
 */

#include <inttypes.h>
#include <stdio.h>

#include "lib/adt/avltree.h"

struct node {
    struct avlnode info;
    uint32_t number;
};

static int compare(struct avlnode *const ours, struct avlnode *const theirs) {
    struct node *const our_node = container_of(ours, struct node, info);
    struct node *const their_node = container_of(theirs, struct node, info);

    return (int64_t)our_node->number - their_node->number;
}

static int identify(struct avlnode *const theirs, void *const key) {
    struct node *const their_node = container_of(theirs, struct node, info);
    return (int64_t)key - (int64_t)their_node->number;
}

static void insert_node(struct avltree *const tree, const uint32_t number) {
    struct node *const avl_node = malloc(sizeof(struct node));
    avl_node->number = number;

    const bool result =
        avltree_insert(tree,
                       (struct avlnode *)avl_node,
                       compare,
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
    const avlnode_compare_key_t compare_identity = identify;

    free(avltree_delete(&tree, (void *)53, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)11, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)21, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)9, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)8, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)61, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)33, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)73, compare_identity, NULL));
    free(avltree_delete(&tree, (void *)71, compare_identity, NULL));

    printf("After deleting, tree should be null\n");
    assert(tree.root == NULL);
}