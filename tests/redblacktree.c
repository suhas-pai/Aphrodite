/*
 * tests/redblacktree.c
 * © suhas pai
 */

#include <lib/adt/redblacktree.h>

struct node {
    struct redblacktree_node info;
    int number;
};

static int
compare(struct redblacktree_node *const ours,
        struct redblacktree_node *const theirs,
        void *const cb_info)
{
    struct node *const our_node = parent_of(ours, struct node, info);
    struct node *const their_node = parent_of(theirs, struct node, info);

    return (int64_t)our_node->number - their_node->number;
}

static int
identify(struct redblacktree_node *const theirs,
         void *const key,
         void *const cb_info)
{
    (void)cb_info;

    struct node *const their_node = parent_of(theirs, struct node, info);
    return (int64_t)key - (int64_t)their_node->number;
}

static void insert_node(struct redblacktree *const tree, const uint32_t number) {
    struct node *const avl_node = malloc(sizeof(struct node));
    avl_node->number = number;

    const bool result =
        redblacktree_insert(tree,
                            (struct redblacktree_node *)avl_node,
                            compare,
                            /*moved=*/nullptr,
                            /*added_node=*/nullptr,
                            /*cb_info=*/nullptr);

    assert(result);
}

void test_redblacktree() {
    struct redblacktree tree = REDBLACKTREE_INIT();

    insert_node(&tree, 8);
    insert_node(&tree, 9);
    insert_node(&tree, 11);
    insert_node(&tree, 21);
    insert_node(&tree, 33);
    insert_node(&tree, 53);
    insert_node(&tree, 61);
    insert_node(&tree, 73);
    insert_node(&tree, 71);

    assert(redblacktree_find(&tree, (void *)33, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)73, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)9, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)21, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)53, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)61, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)71, identify, nullptr) != nullptr);
    assert(redblacktree_find(&tree, (void *)8, identify, nullptr) != nullptr);

    assert(redblacktree_delete(&tree, (void *)53, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)11, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)21, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)9, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)8, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)61, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)33, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)73, identify, nullptr, nullptr));
    assert(redblacktree_delete(&tree, (void *)71, identify, nullptr, nullptr));

    assert(tree.root == nullptr);
}
