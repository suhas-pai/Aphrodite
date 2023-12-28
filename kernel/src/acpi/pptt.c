/*
 * kernel/src/acpi/pptt.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/util.h"

#include "pptt.h"

void pptt_init(const struct acpi_pptt *const pptt) {
    uint32_t offset = offsetof(struct acpi_pptt, buffer);
    while (index_in_bounds(offset, pptt->sdt.length)) {
        struct acpi_pptt_node_base *const base =
            (struct acpi_pptt_node_base *)((uint64_t)pptt + offset);

        switch (base->kind) {
            case ACPI_PPTT_NODE_PROCESSOR_HIERARCHY: {
                struct acpi_pptt_processor_hierarchy_node *const node =
                    (struct acpi_pptt_processor_hierarchy_node *)base;

                offset += sizeof(*node);
                if (!ordinal_in_bounds(offset, pptt->sdt.length)) {
                    printk(LOGLEVEL_WARN,
                           "pptt: processor-hierarchy node goes beyond end "
                           "of node\n");
                    return;
                }

                printk(LOGLEVEL_INFO,
                       "pptt: processor-hierarchy node\n"
                       "\tlength: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\t\tphysical package: %s\n"
                       "\t\tacpi id valid: %s\n"
                       "\t\tprocessor is thread: %s\n"
                       "\t\tnode is leaf: %s\n"
                       "\t\tidentical implementation: %s\n"
                       "\tparent-offset: 0x%" PRIx32 "\n"
                       "\tacpi processor-id: %" PRIu32 "\n"
                       "\tprivate resource count: %" PRIu32 "\n",
                       node->length,
                       node->flags,
                       node->flags &
                        __ACPI_PPTT_PROCESSOR_HIERARCHY_NODE_PHYSICAL_PKG ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_PROCESSOR_HIERARCHY_ACPI_ID_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_PROCESSOR_HIERARCHY_PROCESSOR_IS_THREAD ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_PROCESSOR_HIERARCHY_NODE_IS_LEAF ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_PROCESSOR_HIERARCHY_IDENTICAL_IMPL ?
                            "yes" : "no",
                       node->parent_offset,
                       node->acpi_processor_id,
                       node->private_resource_count);

                if (node->private_resource_count != 0) {
                    printk(LOGLEVEL_INFO, "\tprivate resource offsets:\n");
                    for (uint32_t i = 0; i != node->private_resource_count; i++)
                    {
                        printk(LOGLEVEL_INFO,
                               "\t\t%" PRIu32 ". 0x%" PRIx32 "\n",
                               i + 1,
                               node->private_resource_offsets[i]);
                    }
                }

                break;
            }
            case ACPI_PPTT_NODE_CACHE_TYPE: {
                struct acpi_pptt_cache_type_node *const node =
                    (struct acpi_pptt_cache_type_node *)base;

                offset += sizeof(*node);
                if (!ordinal_in_bounds(offset, pptt->sdt.length)) {
                    printk(LOGLEVEL_WARN,
                           "pptt: cache-type node goes beyond end of node\n");
                    return;
                }

                const enum acpi_pptt_cache_type_node_attr_alloc_kind
                    alloc_kind =
                        node->attributes &
                            __ACPI_PPTT_CACHE_TYPE_NODE_ATTR_WRITE_ALLOC_KIND;

                const char *alloc_kind_str = "unknown";
                switch (alloc_kind) {
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_ALLOC_KIND_READ_ALLOC:
                        alloc_kind_str = "read-alloc";
                        break;
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_ALLOC_KIND_WRITE_ALLOC:
                        alloc_kind_str = "write-alloc";
                        break;
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_ALLOC_KIND_RDWR_ALLOC:
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_ALLOC_KIND_RDWR_ALLOC_2:
                        alloc_kind_str = "read-write-alloc";
                        break;
                }

                const enum acpi_pptt_cache_type_node_attr_cache_kind
                    cache_kind =
                        node->attributes &
                            __ACPI_PPTT_CACHE_TYPE_NODE_ATTR_WRITE_CACHE_KIND;

                const char *cache_kind_str = "unknown";
                switch (cache_kind) {
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_CACHE_KIND_DATA:
                        cache_kind_str = "data";
                        break;
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_CACHE_KIND_INSTRUCTION:
                        cache_kind_str = "instruction";
                        break;
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_CACHE_KIND_UNIFIED:
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_CACHE_KIND_UNIFIED_2:
                        cache_kind_str = "unified";
                        break;
                }

                const enum acpi_pptt_cache_type_node_attr_write_policy
                    wr_policy =
                        node->attributes &
                            __ACPI_PPTT_CACHE_TYPE_NODE_ATTR_WRITE_CACHE_KIND;

                const char *wr_policy_str = "unknown";
                switch (wr_policy) {
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_WRITE_POLICY_BACK:
                        wr_policy_str = "write-back";
                        break;
                    case ACPI_PPTT_CACHE_TYPE_NODE_ATTR_CACHE_KIND_INSTRUCTION:
                        wr_policy_str = "write-through";
                        break;
                }

                printk(LOGLEVEL_INFO,
                       "pptt: cache-type node\n"
                       "\tlength: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\t\tsize valid: %s\n"
                       "\t\tset-count valid: %s\n"
                       "\t\tassociativity valid: %s\n"
                       "\t\talloc-kind valid: %s\n"
                       "\t\tcache-kind valid: %s\n"
                       "\t\twrite-policy valid: %s\n"
                       "\t\tline-size valid: %s\n"
                       "\t\tcache-id valid: %s\n"
                       "\tcache next level: %" PRIu32 "\n"
                       "\tsize: %" PRIu32 " bytes\n"
                       "\tset-count: %" PRIu32 "\n"
                       "\tassociativity: %" PRIu8 "\n"
                       "\tattributes: 0x%" PRIx8 "\n"
                       "\t\talloc-kind: %s\n"
                       "\t\tcache-kind: %s\n"
                       "\t\twrite-policy: %s\n"
                       "\tline-size: %" PRIu16 " bytes\n"
                       "\tcache-id: %" PRIu32 "\n",
                       node->length,
                       node->flags,
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_SIZE_VALID ? "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_SET_COUNT_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_ASSOC_VALID ? "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_ALLOC_KIND_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_CACHE_KIND_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_WRITE_POLICY_KIND_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_LINE_SIZE_VALID ?
                            "yes" : "no",
                       node->flags &
                        __ACPI_PPTT_CACHE_TYPE_NODE_CACHE_ID_VALID ?
                            "yes" : "no",
                       node->cache_next_level,
                       node->size,
                       node->set_count,
                       node->associativity,
                       node->attributes,
                       alloc_kind_str,
                       cache_kind_str,
                       wr_policy_str,
                       node->line_size,
                       node->cache_id);
                break;
            }
            default:
                printk(LOGLEVEL_WARN,
                       "pptt: unrecognized node: %" PRIu8 "\n",
                       base->kind);
                return;
        }
    }
}