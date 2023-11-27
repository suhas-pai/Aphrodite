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
    root->known_props = ARRAY_INIT(sizeof(struct devicetree_prop *));
    root->other_props = ARRAY_INIT(sizeof(struct devicetree_prop_other *));

    tree->root = root;
    tree->phandle_list = ARRAY_INIT(sizeof(struct devicetree_prop *));

    return tree;
}

__optimize(3) struct devicetree_node *
devicetree_get_node_for_phandle(struct devicetree *const tree,
                                const uint32_t phandle)
{
    array_foreach(&tree->phandle_list, struct devicetree_node *, iter) {
        struct devicetree_node *const node = *iter;
        array_foreach(&node->known_props, struct devicetree_prop *, prop_iter) {
            const struct devicetree_prop *const prop = *prop_iter;
            if (prop->kind == DEVICETREE_PROP_PHANDLE) {
                const struct devicetree_prop_phandle *const phandle_prop =
                    (const struct devicetree_prop_phandle *)prop;

                if (phandle_prop->phandle == phandle) {
                    return node;
                }

                break;
            }
        }
    }

    return NULL;
}

void devicetree_node_free(struct devicetree_node *const node) {
    array_foreach(&node->known_props, struct devicetree_prop *, iter) {
        struct devicetree_prop *const prop = *iter;
        switch (prop->kind) {
            case DEVICETREE_PROP_COMPAT:
                break;
            case DEVICETREE_PROP_REG: {
                struct devicetree_prop_reg *const reg_prop =
                    (struct devicetree_prop_reg *)(uint64_t)prop;

                array_destroy(&reg_prop->list);
                break;
            }
            case DEVICETREE_PROP_RANGES:
            case DEVICETREE_PROP_DMA_RANGES: {
                struct devicetree_prop_ranges *const ranges_prop =
                    (struct devicetree_prop_ranges *)(uint64_t)prop;

                array_destroy(&ranges_prop->list);
                break;
            }
            case DEVICETREE_PROP_MODEL:
            case DEVICETREE_PROP_STATUS:
            case DEVICETREE_PROP_ADDR_SIZE_CELLS:
            case DEVICETREE_PROP_PHANDLE:
            case DEVICETREE_PROP_VIRTUAL_REG:
            case DEVICETREE_PROP_DMA_COHERENT:
                break;
            case DEVICETREE_PROP_INTERRUPTS: {
                struct devicetree_prop_interrupts *const int_prop =
                    (struct devicetree_prop_interrupts *)(uint64_t)prop;

                array_destroy(&int_prop->list);
                break;
            }
            case DEVICETREE_PROP_INTERRUPT_MAP: {
                struct devicetree_prop_interrupt_map *const map_prop =
                    (struct devicetree_prop_interrupt_map *)(uint64_t)prop;

                array_destroy(&map_prop->list);
                break;
            }
            case DEVICETREE_PROP_INTERRUPT_PARENT:
            case DEVICETREE_PROP_INTERRUPT_CELLS:
                break;
            case DEVICETREE_PROP_INTERRUPT_MAP_MASK: {
                struct devicetree_prop_interrupt_map_mask *const map_prop =
                    (struct devicetree_prop_interrupt_map_mask *)(uint64_t)prop;

                array_destroy(&map_prop->list);
                break;
            }
            case DEVICETREE_PROP_SPECIFIER_MAP: {
                struct devicetree_prop_specifier_map *const map_prop =
                    (struct devicetree_prop_specifier_map *)(uint64_t)prop;

                array_destroy(&map_prop->list);
                break;
            }
            case DEVICETREE_PROP_SPECIFIER_CELLS:
            case DEVICETREE_PROP_SERIAL_CLOCK_FREQ:
            case DEVICETREE_PROP_SERIAL_CURRENT_SPEED:
                break;
        }
    }

    array_destroy(&node->known_props);
    array_destroy(&node->other_props);

    struct devicetree_node *iter = NULL;
    list_foreach(iter, &node->child_list, sibling_list) {
        devicetree_node_free(iter);
    }
}

__optimize(3) void devicetree_free(struct devicetree *const tree) {
    devicetree_node_free(tree->root);
    array_destroy(&tree->phandle_list);

    kfree(tree->root);
    kfree(tree);
}