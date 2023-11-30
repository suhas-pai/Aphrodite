/*
 * kernel/src/dev/dtb/node.c
 * © suhas pai
 */

#include "node.h"

__optimize(3) struct string_view
devicetree_prop_other_get_sv(const struct devicetree_prop_other *const prop) {
    const char *const ptr = (const char *)prop->data;
    return sv_create_length(ptr, strnlen(ptr, prop->data_length));
}

__optimize(3) const struct devicetree_prop *
devicetree_node_get_prop(const struct devicetree_node *const node,
                         const enum devicetree_prop_kind kind)
{
    array_foreach(&node->known_props, const struct devicetree_prop *, iter) {
        const struct devicetree_prop *const prop = *iter;
        if (prop->kind == kind) {
            return prop;
        }
    }

    return NULL;
}

__optimize(3) const struct devicetree_prop_other *
devicetree_node_get_other_prop(const struct devicetree_node *const node,
                               const struct string_view name)
{
    array_foreach(&node->other_props,
                  const struct devicetree_prop_other *,
                  iter)
    {
        const struct devicetree_prop_other *const prop = *iter;
        if (sv_equals(prop->name, name)) {
            return prop;
        }
    }

    return NULL;
}

static bool
fdt_stringlist_contains_sv(const char *strlist,
                           uint32_t listlen,
                           const struct string_view sv)
{
    while (listlen >= sv.length) {
        if (memcmp(sv.begin, strlist, (size_t)sv.length + 1) == 0) {
            return true;
        }

        const char *const p = memchr(strlist, '\0', (size_t)listlen);
        if (!p) {
            return false; /* malformed strlist.. */
        }

        listlen -= distance(strlist, p) + 1;
        strlist = p + 1;
    }

    return false;
}

bool
devicetree_prop_compat_has_sv(const struct devicetree_prop_compat *const prop,
                              const struct string_view sv)
{
    const bool result =
        fdt_stringlist_contains_sv(prop->string.begin, prop->string.length, sv);

    return result;
}