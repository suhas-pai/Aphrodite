/*
 * kernel/src/arch/riscv64/acpi/rhct.c
 * © suahs pai
 */

#include "dev/printk.h"
#include "lib/util.h"
#include "rhct.h"

void
print_rhct_node(const struct acpi_rhct *const rhct,
                struct acpi_rhct_node *const node,
                const char *const prefix)
{
    switch (node->kind) {
        case ACPI_RHCT_NODE_KIND_ISA_STRING: {
            struct acpi_rhct_isa_string *const isa_str =
                (struct acpi_rhct_isa_string *)node;
            const struct string_view isa_sv =
                sv_create_length(isa_str->isa_string, isa_str->isa_length);

            printk(LOGLEVEL_INFO,
                   "%srhct: found isa string:\n"
                   "%s\tisa length: %" PRIu16 "\n"
                   "%s\tisa string: \"" SV_FMT "\"\n",
                   prefix,
                   prefix, isa_str->isa_length,
                   prefix, SV_FMT_ARGS(isa_sv));
            break;
        }
        case ACPI_RHCT_NODE_KIND_CMO: {
            struct acpi_rhct_cmo_node *const cmo_node =
                (struct acpi_rhct_cmo_node *)node;

            if (!index_in_bounds(cmo_node->cbom_size, sizeof_bits(uint64_t)) ||
                !index_in_bounds(cmo_node->cbop_size, sizeof_bits(uint64_t)) ||
                !index_in_bounds(cmo_node->cboz_size, sizeof_bits(uint64_t)))
            {
                printk(LOGLEVEL_WARN,
                       "%srhct: cmo-node has invalid fields\n",
                       prefix);
                break;
            }

            printk(LOGLEVEL_INFO,
                   "%srhct: found cmo node:\n"
                   "%s\tcbom size: %" PRIu8 "\n"
                   "%s\tcbop size: %" PRIu8 "\n"
                   "%s\tcboz size: %" PRIu8 "\n",
                   prefix,
                   prefix, cmo_node->cbom_size,
                   prefix, cmo_node->cbop_size,
                   prefix, cmo_node->cboz_size);

            break;
        }
        case ACPI_RHCT_NODE_KIND_MMU: {
            struct acpi_rhct_mmu_node *const mmu_node =
                (struct acpi_rhct_mmu_node *)node;

            const char *mmu_kind = "unknown";
            switch ((enum acpi_rhct_mmu_kind)mmu_node->mmu_kind) {
                case ACPI_RHCT_MMU_KIND_SV39:
                    mmu_kind = "sv39";
                    break;
                case ACPI_RHCT_MMU_KIND_SV48:
                    mmu_kind = "sv48";
                    break;
                case ACPI_RHCT_MMU_KIND_SV57:
                    mmu_kind = "sv57";
                    break;
            }

            printk(LOGLEVEL_INFO,
                   "%srhct: found mmu node:\n"
                   "%s\tcbom size: %s\n",
                   prefix,
                   prefix, mmu_kind);

            break;
        }
        case ACPI_RHCT_NODE_KIND_HART_INFO: {
            struct acpi_rhct_hart_info *const hart =
                (struct acpi_rhct_hart_info *)node;

            printk(LOGLEVEL_INFO,
                   "%srhct: found hart info:\n"
                   "%s\toffset count: %" PRIu16 "\n"
                   "%s\tacpi processor uid: %" PRIu32 "\n",
                   prefix,
                   prefix, hart->offset_count,
                   prefix, hart->acpi_processor_uid);

            for (uint16_t j = 0; j != hart->offset_count; j++) {
                struct acpi_rhct_node *const hart_node =
                    reg_to_ptr(struct acpi_rhct_node, rhct, hart->offsets[j]);

                print_rhct_node(rhct, hart_node, "\t\t");
            }

            break;
        }
    }
}

void acpi_rhct_init(const struct acpi_rhct *const rhct) {
    printk(LOGLEVEL_INFO,
           "rhct:\n"
           "\ttime base frequency: %" PRIu64 "\n"
           "\tnode count: %" PRIu32 "\n"
           "\tnode offset: %" PRIu32 "\n",
           rhct->time_base_freq,
           rhct->node_count,
           rhct->node_offset);

    struct acpi_rhct_node *node =
        reg_to_ptr(struct acpi_rhct_node, rhct, rhct->node_offset);

    for (uint32_t i = 0; i != rhct->node_count; i++) {
        print_rhct_node(rhct, node, /*prefix=*/"");
        node = reg_to_ptr(struct acpi_rhct_node, node, node->length);
    }
}