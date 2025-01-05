/*
 * kernel/src/acpi/pptt.c
 * © suhas pai
 */

#include "acpi/pptt.h"
#include "dev/printk.h"
#include "lib/util.h"

void pptt_init(const struct os_acpi_pptt *const pptt) {
    uint32_t offset = offsetof(struct os_acpi_pptt, buffer);
    while (index_in_bounds(offset, pptt->sdt.length)) {
        const auto base =
            reg_to_ptr(struct os_acpi_pptt_node_base, pptt, offset);

        switch (base->kind) {
            case ACPI_PPTT_NODE_PROCESSOR_HIERARCHY: {
                const auto node =
                    (struct os_acpi_pptt_processor_hierarchy_node *)base;

                offset += sizeof(*node);
                if (!ordinal_in_bounds(offset, pptt->sdt.length)) {
                    printk(LOGLEVEL_WARN,
                           "pptt: processor-hierarchy node goes beyond end of "
                           "node\n");
                    return;
                }

                printk(LOGLEVEL_INFO,
                       "pptt: processor-hierarchy node\n"
                       "\t\tlength: %" PRIu32 "\n"
                       "\t\tflags: 0x%" PRIx32 "\n"
                       "\t\t\tphysical package: %s\n"
                       "\t\t\tacpi id valid: %s\n"
                       "\t\t\tprocessor is thread: %s\n"
                       "\t\t\tnode is leaf: %s\n"
                       "\t\t\tidentical implementation: %s\n"
                       "\t\tparent-offset: 0x%" PRIx32 "\n"
                       "\t\tacpi processor-id: %" PRIu32 "\n"
                       "\t\tprivate resource count: %" PRIu32 "\n",
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

                continue;
            }
            case ACPI_PPTT_NODE_CACHE_TYPE: {
                const auto node = (struct os_acpi_pptt_cache_type_node *)base;
                offset += sizeof(*node);

                if (!ordinal_in_bounds(offset, pptt->sdt.length)) {
                    printk(LOGLEVEL_WARN,
                           "pptt: cache-type node goes beyond end of node\n");
                    return;
                }

                const auto alloc_kind =
                    (enum os_acpi_pptt_cache_type_node_attr_alloc_kind)
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

                const auto cache_kind =
                    (enum os_acpi_pptt_cache_type_node_attr_cache_kind)
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

                const auto wr_policy =
                    (enum os_acpi_pptt_cache_type_node_attr_write_policy)
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
                       "\t\tlength: %" PRIu32 "\n"
                       "\t\tflags: 0x%" PRIx32 "\n"
                       "\t\t\tsize valid: %s\n"
                       "\t\t\tset-count valid: %s\n"
                       "\t\t\tassociativity valid: %s\n"
                       "\t\t\talloc-kind valid: %s\n"
                       "\t\t\tcache-kind valid: %s\n"
                       "\t\t\twrite-policy valid: %s\n"
                       "\t\t\tline-size valid: %s\n"
                       "\t\t\tcache-id valid: %s\n"
                       "\t\tcache next level: %" PRIu32 "\n"
                       "\t\tsize: %" PRIu32 " bytes\n"
                       "\t\tset-count: %" PRIu32 "\n"
                       "\t\tassociativity: %" PRIu8 "\n"
                       "\t\tattributes: 0x%" PRIx8 "\n"
                       "\t\t\talloc-kind: %s\n"
                       "\t\t\tcache-kind: %s\n"
                       "\t\t\twrite-policy: %s\n"
                       "\t\tline-size: %" PRIu16 " bytes\n"
                       "\t\tcache-id: %" PRIu32 "\n",
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
                continue;
            }
        }

        printk(LOGLEVEL_WARN,
                "pptt: unrecognized node: %" PRIu8 "\n",
                base->kind);
        return;
    }
}