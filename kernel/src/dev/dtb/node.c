/*
 * kernel/src/dev/dtb/node.c
 * © suhas pai
 */

#include "node.h"

__optimize(3) struct devicetree_prop *
devicetree_node_get_prop(struct devicetree_node *const node,
                         const enum devicetree_prop_kind kind)
{
    array_foreach(&node->known_props, struct devicetree_prop *, iter) {
        struct devicetree_prop *const prop = *iter;
        if (prop->kind == kind) {
            return prop;
        }
    }

    return NULL;
}

__optimize(3) struct devicetree_prop_other *
devicetree_node_get_other_prop(struct devicetree_node *const node,
                               const struct string_view name)
{
    array_foreach(&node->other_props, struct devicetree_prop_other *, iter) {
        struct devicetree_prop_other *const prop = *iter;
        if (sv_equals(prop->name, name)) {
            return prop;
        }
    }

    return NULL;
}