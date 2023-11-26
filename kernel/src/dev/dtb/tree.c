/*
 * kernel/src/dev/dtb/tree.c
 * © suhas pai
 */

#include "dev/dtb/node.h"
#include "mm/kmalloc.h"

#include "tree.h"

struct devicetree *devicetree_alloc() {
    struct devicetree_node *const root = kmalloc(sizeof(*root));
    if (root == NULL) {
        return NULL;
    }

    struct devicetree *const tree = kmalloc(sizeof(*tree));
    if (tree == NULL) {
        kfree(root);
        return NULL;
    }

    list_init(&root->child_list);
    list_init(&root->sibling_list);

    root->name = SV_EMPTY();
    root->parent = NULL;
    root->nodeoff = 0;
    root->known_props = ARRAY_INIT(sizeof(void *));
    root->other_props = ARRAY_INIT(sizeof(struct devicetree_prop_other *));

    tree->root = root;
    tree->phandle_list = ARRAY_INIT(sizeof(void *));

    return tree;
}