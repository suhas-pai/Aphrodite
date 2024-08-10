/*
 * kernel/src/dev/dtb/node.c
 * © suhas pai
 */

#include "node.h"
#define DEVICETREE_PROP_MAP_BUCKET_COUNT 6

__debug_optimize(3) void
devicetree_node_init_fields(struct devicetree_node *const node,
                            struct devicetree_node *const parent,
                            const struct string_view name,
                            const int nodeoff)
{
    node->name = name;
    node->parent = parent;
    node->nodeoff = nodeoff;
    node->known_props =
        HASHMAP_INIT(sizeof(struct devicetree_prop *),
                     DEVICETREE_PROP_MAP_BUCKET_COUNT,
                     hashmap_no_hash,
                     /*hash_cb_info=*/NULL);

    node->other_props = ARRAY_INIT(sizeof(struct devicetree_prop_other *));
}

__debug_optimize(3) bool
devicetree_prop_other_get_u32(const struct devicetree_prop_other *const prop,
                              uint32_t *const result_out)
{
    if (prop->data_length != sizeof(fdt32_t)) {
        return false;
    }

    *result_out = fdt32_to_cpu(*prop->data);
    return true;
}

bool
devicetree_prop_other_get_u32_list(
    const struct devicetree_prop_other *const prop,
    const uint32_t u32_in_elem_count,
    const fdt32_t **const result_out,
    uint32_t *const count_out)
{
    uint32_t elem_size = 0;
    if (!check_mul(u32_in_elem_count, sizeof(uint32_t), &elem_size)) {
        return false;
    }

    if ((prop->data_length % elem_size) != 0) {
        return false;
    }

    *result_out = prop->data;
    *count_out = prop->data_length / elem_size;

    return true;
}

__debug_optimize(3) struct string_view
devicetree_prop_other_get_sv(const struct devicetree_prop_other *const prop) {
    const char *const ptr = (const char *)prop->data;
    return sv_create_length(ptr, strnlen(ptr, prop->data_length));
}

__debug_optimize(3) const struct devicetree_prop *
devicetree_node_get_prop(const struct devicetree_node *const node,
                         const enum devicetree_prop_kind kind)
{
    struct devicetree_prop **const prop_ptr =
        hashmap_get(&node->known_props, hashmap_key_create(kind));

    if (prop_ptr != NULL) {
        return *prop_ptr;
    }

    return NULL;
}

__debug_optimize(3) const struct devicetree_prop_other *
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

__debug_optimize(3) bool
devicetree_node_has_compat_sv(const struct devicetree_node *const node,
                              const struct string_view sv)
{
    const struct devicetree_prop_compat *const compat_prop =
        (const struct devicetree_prop_compat *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_COMPAT);

    if (compat_prop != NULL) {
        return devicetree_prop_compat_has_sv(compat_prop, sv);
    }

    return false;
}

__debug_optimize(3) static bool
fdt_stringlist_contains_sv(const char *strlist,
                           uint32_t listlen,
                           const struct string_view sv)
{
    while (listlen >= sv.length) {
        if (memcmp(sv.begin, strlist, (size_t)sv.length + 1) == 0) {
            return true;
        }

        const char *const p = memchr(strlist, '\0', (size_t)listlen);
        if (p == NULL) {
            return false; /* malformed strlist.. */
        }

        listlen -= distance(strlist, p) + 1;
        strlist = p + 1;
    }

    return false;
}

__debug_optimize(3) bool
devicetree_prop_compat_has_sv(const struct devicetree_prop_compat *const prop,
                              const struct string_view sv)
{
    const struct string_view prop_sv = prop->string;
    return fdt_stringlist_contains_sv(prop_sv.begin, prop_sv.length, sv);
}