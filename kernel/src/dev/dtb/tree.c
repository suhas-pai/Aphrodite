/*
 * kernel/src/dev/dtb/tree.c
 * © suhas pai
 */

#include <lib/adt/string_view.h>
#include <lib/path.h>

#include "dev/dtb/tree.h"
#include "mm/kmalloc.h"

#define DEVICETREE_PHANDLE_MAP_BUCKET_COUNT 10

__debug_optimize(3) struct devicetree *devicetree_alloc() {
    struct devicetree *const tree = kmalloc(sizeof(*tree));
    if (tree == nullptr) {
        return nullptr;
    }

    devicetree_init_fields(tree, /*root=*/nullptr);
    struct devicetree_node *const root =
        simple_alloc(&tree->alloc, sizeof(*root));

    if (root == nullptr) {
        kfree(tree);
        return nullptr;
    }

    devicetree_node_init_fields(root,
                                /*parent=*/nullptr,
                                /*name=*/SV_EMPTY(),
                                /*nodeoff=*/0);

    return tree;
}

__debug_optimize(3) void
devicetree_init_fields(struct devicetree *const tree,
                       struct devicetree_node *const root)
{
    simple_alloc_init(&tree->alloc);

    tree->root = root;
    tree->phandle_map =
        HASHMAP_INIT(sizeof(struct devicetree_node *),
                     DEVICETREE_PHANDLE_MAP_BUCKET_COUNT,
                     hashmap_no_hash,
                     /*hash_cb_info=*/nullptr);
}

__debug_optimize(3) const struct devicetree_node *
devicetree_get_node_for_phandle(const struct devicetree *const tree,
                                const uint32_t phandle)
{
    const struct devicetree_node *const *const node_ptr =
        hashmap_get(&tree->phandle_map, hashmap_key_create(phandle));

    if (node_ptr != nullptr) {
        return *node_ptr;
    }

    return nullptr;
}

struct devicetree_node *
devicetree_get_node_at_path(const struct devicetree *const tree,
                            const struct string_view path)
{
    if (path_sv_is_relative(path)) {
        return nullptr;
    }

    struct devicetree_node *node = tree->root;
    path_sv_foreach_component(path, component_sv, /*skip_root=*/true) {
        bool found = false;
        devicetree_node_foreach_child(node, iter) {
            if (sv_equals(iter->name, component_sv)) {
                found = true;
                break;
            }
        }

        if (found) {
            node = iter;
            continue;
        }

        break;
    }

    return nullptr;
}

void
devicetree_node_free(struct devicetree *const tree,
                     struct devicetree_node *const node)
{
    hashmap_foreach_bucket(&node->known_props, bucket_ptr) {
        struct hashmap_bucket *const bucket = *bucket_ptr;
        if (bucket == nullptr) {
            continue;
        }

        hashmap_bucket_foreach_node(bucket, map_node) {
            struct devicetree_prop **const prop_ptr =
                cast_to_ptr(struct devicetree_prop *, map_node->data);

            struct devicetree_prop *const prop = *prop_ptr;
            switch (prop->kind) {
                case DEVICETREE_PROP_COMPAT:
                    goto free_prop;
                case DEVICETREE_PROP_REG: {
                    struct devicetree_prop_reg *const reg_prop =
                        cast_to_ptr(struct devicetree_prop_reg, prop);

                    array_destroy(&reg_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_RANGES:
                case DEVICETREE_PROP_DMA_RANGES: {
                    struct devicetree_prop_ranges *const ranges_prop =
                        cast_to_ptr(struct devicetree_prop_ranges, prop);

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
                        cast_to_ptr(struct devicetree_prop_interrupts, prop);

                    array_destroy(&intr_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_INTR_MAP: {
                    struct devicetree_prop_intr_map *const map_prop =
                        cast_to_ptr(struct devicetree_prop_intr_map, prop);

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_INTR_PARENT:
                case DEVICETREE_PROP_INTR_CONTROLLER:
                case DEVICETREE_PROP_INTR_CELLS:
                    goto free_prop;
                case DEVICETREE_PROP_INTR_MAP_MASK: {
                    struct devicetree_prop_intr_map_mask *const map_prop =
                        cast_to_ptr(struct devicetree_prop_intr_map_mask, prop);

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_MSI_CONTROLLER:
                    goto free_prop;
                case DEVICETREE_PROP_SPECIFIER_MAP: {
                    struct devicetree_prop_specifier_map *const map_prop =
                        cast_to_ptr(struct devicetree_prop_specifier_map, prop);

                    array_destroy(&map_prop->list);
                    goto free_prop;
                }
                case DEVICETREE_PROP_SPECIFIER_CELLS:
                case DEVICETREE_PROP_SERIAL_CLOCK_FREQ:
                case DEVICETREE_PROP_SERIAL_CURRENT_SPEED:
                case DEVICETREE_PROP_PCI_BUS_RANGE:
                    goto free_prop;

                free_prop:
                    simple_free(&tree->alloc, prop);
                    continue;
            }

            verify_not_reached();
        }
    }

    hashmap_destroy(&node->known_props);
    array_destroy(&node->other_props);

    devicetree_node_foreach_child(node, iter) {
        devicetree_node_free(tree, iter);
    }
}

__debug_optimize(3) void devicetree_free(struct devicetree *const tree) {
    devicetree_node_free(tree, tree->root);

    hashmap_destroy(&tree->phandle_map);
    simple_alloc_destroy(&tree->alloc);

    kfree(tree->root);
    kfree(tree);
}