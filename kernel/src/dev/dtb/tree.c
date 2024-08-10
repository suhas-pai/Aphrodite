/*
 * kernel/src/dev/dtb/tree.c
 * © suhas pai
 */

#include "lib/adt/string_view.h"
#include "mm/kmalloc.h"

#include "tree.h"

#define DEVICETREE_PHANDLE_MAP_BUCKET_COUNT 10

__debug_optimize(3) struct devicetree *devicetree_alloc() {
    struct devicetree_node *const root = kmalloc(sizeof(*root));
    if (root == NULL) {
        return NULL;
    }

    struct devicetree *const tree = kmalloc(sizeof(*tree));
    if (tree == NULL) {
        kfree(root);
        return NULL;
    }

    devicetree_node_init_fields(root,
                                /*parent=*/NULL,
                                /*name=*/SV_EMPTY(),
                                /*nodeoff=*/0);

    devicetree_init_fields(tree, root);
    return tree;
}

__debug_optimize(3) void
devicetree_init_fields(struct devicetree *const tree,
                       struct devicetree_node *const root)
{
    list_init(&root->child_list);
    list_init(&root->sibling_list);

    tree->root = root;
    tree->phandle_map =
        HASHMAP_INIT(sizeof(struct devicetree_node *),
                     DEVICETREE_PHANDLE_MAP_BUCKET_COUNT,
                     hashmap_no_hash,
                     /*hash_cb_info=*/NULL);
}

__debug_optimize(3) const struct devicetree_node *
devicetree_get_node_for_phandle(const struct devicetree *const tree,
                                const uint32_t phandle)
{
    const struct devicetree_node *const *const node_ptr =
        hashmap_get(&tree->phandle_map, hashmap_key_create(phandle));

    if (node_ptr != NULL) {
        return *node_ptr;
    }

    return NULL;
}

struct devicetree_node *
devicetree_get_node_at_path(const struct devicetree *const tree,
                            const struct string_view path)
{
    if (__builtin_expect(path.length == 0 || sv_front(path) != '/', 0)) {
        return NULL;
    }

    struct devicetree_node *node = tree->root;
    if (path.length == 1) {
        return node;
    }

    if (__builtin_expect(node == NULL, 0)) {
        return NULL;
    }

    uint32_t component_begin = 1;
    do {
        int64_t component_length = sv_find_char(path, component_begin, '/');
        if (component_length != -1) {
            component_length -= component_begin;
            if (__builtin_expect(component_length == 0, 0)) {
                // `//` component found in path string.
                return NULL;
            }
        } else {
            component_length = path.length - component_begin;
        }

        const struct string_view component_sv =
            sv_substring_length(path, component_begin, component_length);

        bool found = false;
        devicetree_node_foreach_child(node, iter) {
            if (sv_equals(iter->name, component_sv)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return NULL;
        }

        if (component_begin + component_length == path.length) {
            return iter;
        }

        component_begin = component_begin + component_length + 1;
        node = iter;
    } while (true);

    return NULL;
}

void devicetree_node_free(struct devicetree_node *const node) {
    hashmap_foreach_bucket(&node->known_props, bucket_ptr) {
        struct hashmap_bucket *const bucket = *bucket_ptr;
        if (bucket == NULL) {
            continue;
        }

        hashmap_bucket_foreach_node(bucket, map_node) {
            struct devicetree_prop **const prop_ptr =
                (struct devicetree_prop **)(uint64_t)map_node->data;

            struct devicetree_prop *const prop = *prop_ptr;
            switch (prop->kind) {
                case DEVICETREE_PROP_COMPAT:
                    goto free_prop;
                case DEVICETREE_PROP_REG: {
                    struct devicetree_prop_reg *const reg_prop =
                        (struct devicetree_prop_reg *)(uint64_t)prop;

                    array_destroy(&reg_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_RANGES:
                case DEVICETREE_PROP_DMA_RANGES: {
                    struct devicetree_prop_ranges *const ranges_prop =
                        (struct devicetree_prop_ranges *)(uint64_t)prop;

                    array_destroy(&ranges_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_MODEL:
                case DEVICETREE_PROP_STATUS:
                case DEVICETREE_PROP_ADDR_SIZE_CELLS:
                case DEVICETREE_PROP_PHANDLE:
                case DEVICETREE_PROP_VIRTUAL_REG:
                case DEVICETREE_PROP_DMA_COHERENT:
                case DEVICETREE_PROP_DEVICE_TYPE:
                    goto free_prop;
                case DEVICETREE_PROP_INTERRUPTS: {
                    struct devicetree_prop_interrupts *const intr_prop =
                        (struct devicetree_prop_interrupts *)(uint64_t)prop;

                    array_destroy(&intr_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_INTR_MAP: {
                    struct devicetree_prop_intr_map *const map_prop =
                        (struct devicetree_prop_intr_map *)(uint64_t)prop;

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_INTR_PARENT:
                case DEVICETREE_PROP_INTR_CONTROLLER:
                case DEVICETREE_PROP_INTR_CELLS:
                    goto free_prop;
                case DEVICETREE_PROP_INTR_MAP_MASK: {
                    struct devicetree_prop_intr_map_mask *const map_prop =
                        (struct devicetree_prop_intr_map_mask *)
                            (uint64_t)prop;

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_MSI_CONTROLLER:
                    goto free_prop;
                case DEVICETREE_PROP_SPECIFIER_MAP: {
                    struct devicetree_prop_specifier_map *const map_prop =
                        (struct devicetree_prop_specifier_map *)(uint64_t)prop;

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_SPECIFIER_CELLS:
                case DEVICETREE_PROP_SERIAL_CLOCK_FREQ:
                case DEVICETREE_PROP_SERIAL_CURRENT_SPEED:
                case DEVICETREE_PROP_PCI_BUS_RANGE:
                    goto free_prop;

                free_prop:
                    kfree(prop);
                    continue;
            }

            verify_not_reached();
        }
    }

    hashmap_destroy(&node->known_props);
    array_destroy(&node->other_props);

    devicetree_node_foreach_child(node, iter) {
        devicetree_node_free(iter);
    }
}

__debug_optimize(3) void devicetree_free(struct devicetree *const tree) {
    devicetree_node_free(tree->root);
    hashmap_destroy(&tree->phandle_map);

    kfree(tree->root);
    kfree(tree);
}