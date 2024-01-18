/*
 * kernel/src/dev/pci/entity.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/msr.h"
    #include "dev/printk.h"

    #include "lib/align.h"
    #include "lib/bits.h"
    #include "lib/util.h"

    #include "sys/mmio.h"
#endif /* defined(__x86_64__) */

#include "dev/printk.h"

#include "entity.h"
#include "structs.h"

void pci_entity_enable_msi(struct pci_entity_info *const entity) {
    if (entity->msi_support == PCI_ENTITY_MSI_SUPPORT_NONE) {
        printk(LOGLEVEL_WARN,
               "pci: pci_entity_enable_msi() called on entity that doesn't "
               "support msi\n");
        return;
    }

    const uint32_t msi_control =
        pci_read_from_base(entity,
                           entity->pcie_msi_offset,
                           struct pci_spec_cap_msi,
                           msg_control);

    pci_write_from_base(entity,
                        entity->pcie_msi_offset,
                        struct pci_spec_cap_msi,
                        msg_control,
                        msi_control | __PCI_CAPMSI_CTRL_ENABLE);
}

#if defined(__x86_64__)
    static void
    bind_msi_to_vector(const struct pci_entity_info *const entity,
                       const uint64_t address,
                       const isr_vector_t vector,
                       const bool masked)
    {
        uint16_t msg_control =
            pci_read_from_base(entity,
                               entity->pcie_msi_offset,
                               struct pci_spec_cap_msi,
                               msg_control);

        const bool supports_masking =
            msg_control & __PCI_CAPMSI_CTRL_PER_VECTOR_MASK;

        // If we're supposed to mask the vector, but masking isn't supported,
        // then simply bail.

        if (masked && !supports_masking) {
            return;
        }

        pci_write_from_base(entity,
                            entity->pcie_msi_offset,
                            struct pci_spec_cap_msi,
                            msg_address,
                            (uint32_t)address);

        const bool is_64_bit = msg_control & __PCI_CAPMSI_CTRL_64BIT_CAPABLE;
        if (is_64_bit) {
            pci_write_from_base(entity,
                                entity->pcie_msi_offset,
                                struct pci_spec_cap_msi,
                                bits64.msg_data,
                                vector);
        } else {
            pci_write_from_base(entity,
                                entity->pcie_msi_offset,
                                struct pci_spec_cap_msi,
                                bits32.msg_data,
                                vector);
        }

        msg_control |= __PCI_CAPMSI_CTRL_ENABLE;
        msg_control = rm_mask(msg_control, __PCI_CAPMSI_CTRL_MULTIMSG_ENABLE);

        pci_write_from_base(entity,
                            entity->pcie_msi_offset,
                            struct pci_spec_cap_msi,
                            msg_control,
                            msg_control);

        if (masked) {
            if (is_64_bit) {
                pci_write_from_base(entity,
                                    entity->pcie_msi_offset,
                                    struct pci_spec_cap_msi,
                                    bits64.mask_bits,
                                    1ull << vector);
            } else {
                pci_write_from_base(entity,
                                    entity->pcie_msi_offset,
                                    struct pci_spec_cap_msi,
                                    bits32.mask_bits,
                                    1ull << vector);
            }
        }
    }

    static void
    bind_msix_to_vector(struct pci_entity_info *const entity,
                        const uint64_t address,
                        const isr_vector_t vector,
                        const bool masked)
    {
        const uint64_t msix_vector =
            bitmap_find(&entity->msix_table,
                        /*count=*/1,
                        /*start_index=*/0,
                        /*expected_value=*/false,
                        /*invert=*/true);

        if (msix_vector == FIND_BIT_INVALID) {
            printk(LOGLEVEL_WARN,
                   "pcie: no free msix table entry found for binding address "
                   "%p to msix vector " ISR_VECTOR_FMT "\n",
                   (void *)address,
                   vector);
            return;
        }

        uint16_t msg_control =
            pci_read_from_base(entity,
                               entity->pcie_msix_offset,
                               struct pci_spec_cap_msix,
                               msg_control);

        const uint32_t table_size =
            (msg_control & __PCI_CAP_MSIX_TABLE_SIZE_MASK) + 1;

        if (!index_in_bounds(vector, table_size)) {
            printk(LOGLEVEL_WARN,
                   "pcie: msix table is too small to bind address %p to msix "
                   "vector " ISR_VECTOR_FMT "\n",
                   (void *)address,
                   vector);
            return;
        }

        /*
         * The lower 3 bits of the Table Offset is the BIR.
         *
         * The BIR (Base Index Register) is the index of the BAR that contains
         * the MSI-X Table.
         *
         * The remaining 29 (32-3) bits of the Table Offset is the offset to the
         * MSI-X Table in the BAR.
         */

        const uint32_t table_offset =
            pci_read_from_base(entity,
                               entity->pcie_msix_offset,
                               struct pci_spec_cap_msix,
                               table_offset);

        const uint8_t bar_index = table_offset & __PCI_BARSPEC_TABLE_OFFSET_BIR;
        if (!index_in_bounds(bar_index, entity->max_bar_count)) {
            printk(LOGLEVEL_WARN,
                   "pcie: got invalid bar index while trying to bind address "
                   "%p to msix vector " ISR_VECTOR_FMT "\n",
                   (void *)address,
                   vector);
            return;
        }

        if (!entity->bar_list[bar_index].is_present) {
            printk(LOGLEVEL_WARN,
                   "pcie: encountered non-present bar for msix table while "
                   "trying to bind address %p to msix vector " ISR_VECTOR_FMT
                   "\n",
                   (void *)address,
                   vector);
            return;
        }

        struct pci_entity_bar_info *const bar = &entity->bar_list[bar_index];
        if (!bar->is_mmio) {
            printk(LOGLEVEL_WARN, "pcie: base-address-reg bar is not mmio\n");
            return;
        }

        const uint64_t bar_address = (uint64_t)bar->mmio->base;
        const uint64_t table_addr =
            bar_address + rm_mask(table_offset, __PCI_BARSPEC_TABLE_OFFSET_BIR);

        volatile struct pci_spec_cap_msix_table_entry *const table =
            (volatile struct pci_spec_cap_msix_table_entry *)table_addr;

        mmio_write(&table[msix_vector].msg_address_lower32, (uint32_t)address);
        mmio_write(&table[msix_vector].msg_address_upper32, 0);
        mmio_write(&table[msix_vector].data, vector);
        mmio_write(&table[msix_vector].control, masked);

        // Enable MSI-X after we setup the table-entry
        msg_control |= __PCI_CAP_MSIX_CTRL_ENABLE;
        pci_write_from_base(entity,
                            entity->pcie_msix_offset,
                            struct pci_spec_cap_msix,
                            msg_control,
                            msg_control);
    }

    bool
    pci_entity_bind_msi_to_vector(struct pci_entity_info *const entity,
                                  const struct cpu_info *const cpu,
                                  const isr_vector_t vector,
                                  const bool masked)
    {
        const uint64_t msi_address =
            align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE) |
            cpu->lapic_id << 12;

        switch (entity->msi_support) {
            case PCI_ENTITY_MSI_SUPPORT_NONE:
                printk(LOGLEVEL_WARN,
                       "pcie: entity " PCI_ENTITY_INFO_FMT " does not support "
                       "msi or msix. failing to bind msi(x) to "
                       "vector " ISR_VECTOR_FMT "\n",
                       PCI_ENTITY_INFO_FMT_ARGS(entity),
                       vector);
                return false;
            case PCI_ENTITY_MSI_SUPPORT_MSI:
                bind_msi_to_vector(entity, msi_address, vector, masked);
                return true;
            case PCI_ENTITY_MSI_SUPPORT_MSIX:
                bind_msix_to_vector(entity, msi_address, vector, masked);
                return true;
        }

        verify_not_reached();
    }
#endif /* defined(__x86_64__) */

void
pci_entity_enable_privl(struct pci_entity_info *const entity,
                        const uint16_t privl)
{
    const uint16_t old_command =
        pci_read(entity, struct pci_spec_entity_info_base, command);
    const uint16_t new_command =
        (old_command | privl) ^ __PCI_DEVCMDREG_INT_DISABLE;

    pci_write(entity, struct pci_spec_entity_info_base, command, new_command);
}
