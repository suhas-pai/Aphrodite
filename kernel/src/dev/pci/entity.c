/*
 * kernel/src/dev/pci/entity.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "entity.h"
#include "structs.h"

__debug_optimize(3) uint16_t
pci_entity_get_requester_id(const struct pci_entity_info *const entity) {
    return (uint16_t)entity->loc.bus << 8
         | (uint16_t)entity->loc.slot << 3
         | entity->loc.function;
}

__debug_optimize(3)
bool pci_entity_enable_msi(struct pci_entity_info *const entity) {
    switch (entity->msi_support) {
        case PCI_ENTITY_MSI_SUPPORT_NONE:
            printk(LOGLEVEL_WARN,
                   "pci: pci_entity_enable_msi() called on entity that doesn't "
                   "support msi\n");
            return false;
        case PCI_ENTITY_MSI_SUPPORT_MSI: {
            spin_acquire_preempt_disable(&entity->lock);
            const uint32_t msg_control =
                pci_read_from_base(entity,
                                   entity->msi_pcie_offset,
                                   struct pci_spec_cap_msi,
                                   msg_control);

            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                msg_control,
                                msg_control | __PCI_CAP_MSI_CTRL_ENABLE);

            spin_release_preempt_enable(&entity->lock);
            return true;
        }
        case PCI_ENTITY_MSI_SUPPORT_MSIX: {
            spin_acquire_preempt_disable(&entity->lock);
            const uint32_t msg_control =
                pci_read_from_base(entity,
                                   entity->msi_pcie_offset,
                                   struct pci_spec_cap_msix,
                                   msg_control);

            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msix,
                                msg_control,
                                msg_control | __PCI_CAP_MSIX_CTRL_ENABLE);

            spin_release_preempt_enable(&entity->lock);
            return true;
        }
    }

    verify_not_reached();
}

__debug_optimize(3)
bool pci_entity_disable_msi(struct pci_entity_info *const entity) {
    switch (entity->msi_support) {
        case PCI_ENTITY_MSI_SUPPORT_NONE:
            printk(LOGLEVEL_WARN,
                   "pci: pci_entity_disable_msi() called on entity that "
                   "doesn't support msi\n");
            return false;
        case PCI_ENTITY_MSI_SUPPORT_MSI: {
            spin_acquire_preempt_disable(&entity->lock);

            const uint32_t msg_control =
                pci_read_from_base(entity,
                                   entity->msi_pcie_offset,
                                   struct pci_spec_cap_msi,
                                   msg_control);

            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                msg_control,
                                rm_mask(msg_control,
                                        __PCI_CAP_MSI_CTRL_ENABLE));

            spin_release_preempt_enable(&entity->lock);
            return true;
        }
        case PCI_ENTITY_MSI_SUPPORT_MSIX: {
            spin_acquire_preempt_disable(&entity->lock);

            const uint32_t msg_control =
                pci_read_from_base(entity,
                                   entity->msi_pcie_offset,
                                   struct pci_spec_cap_msix,
                                   msg_control);

            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msix,
                                msg_control,
                                rm_mask(msg_control,
                                        __PCI_CAP_MSIX_CTRL_ENABLE));

            spin_release_preempt_enable(&entity->lock);
            return true;
        }
    }

    verify_not_reached();
}

#if ISR_SUPPORTS_MSI
    static void
    bind_msi_to_vector(const struct pci_entity_info *const entity,
                       const uint64_t address,
                       const isr_vector_t vector,
                       const bool masked)
    {
        // If we're supposed to mask the vector, but masking isn't supported,
        // then simply bail.

        if (masked && !entity->msi.supports_masking) {
            return;
        }

        pci_write_from_base(entity,
                            entity->msi_pcie_offset,
                            struct pci_spec_cap_msi,
                            msg_address,
                            (uint32_t)address);

        if (entity->msi.supports_64bit) {
            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                bits64.msg_data,
                                vector);
        } else {
            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                bits32.msg_data,
                                vector);
        }

        if (masked) {
            if (entity->msi.supports_64bit) {
                pci_write_from_base(entity,
                                    entity->msi_pcie_offset,
                                    struct pci_spec_cap_msi,
                                    bits64.mask_bits,
                                    1ull << vector);
            } else {
                pci_write_from_base(entity,
                                    entity->msi_pcie_offset,
                                    struct pci_spec_cap_msi,
                                    bits32.mask_bits,
                                    1ull << vector);
            }
        }
    }

    static int32_t
    bind_msix_to_vector(struct pci_entity_info *const entity,
                        const uint64_t address,
                        const isr_vector_t vector,
                        const bool masked)
    {
        struct pci_entity_bar_info *const bar = entity->msix.table_bar;
        if (!pci_map_bar(bar)) {
            printk(LOGLEVEL_WARN, "pcie: failed to map msix table bar\n");
            return -1;
        }

        const uint64_t index =
            bitset_find_unset(entity->msix.bitset,
                              entity->msix.table_size,
                              /*invert=*/true);

        if (index == BITSET_INVALID) {
            printk(LOGLEVEL_WARN,
                   "pcie: msix table is used up, can't bind "
                   "vector " ISR_VECTOR_FMT " to address %p\n",
                   vector,
                   (void *)address);
            return -1;
        }

        volatile struct pci_spec_cap_msix_table_entry *const table =
            pci_entity_bar_get_base(bar) + entity->msix.table_offset;

        mmio_write(&table[index].msg_address_lower32, (uint32_t)address);
        mmio_write(&table[index].msg_address_upper32, address >> 32);

        mmio_write(&table[index].data, vector);
        mmio_write(&table[index].control, (uint32_t)masked);

        return index;
    }
#endif /* defined(ISR_SUPPORTS_MSI) */

int32_t
pci_entity_bind_msi_to_vector(struct pci_entity_info *const entity,
                              const struct cpu_info *const cpu,
                              const isr_vector_t vector,
                              const bool masked)
{
#if ISR_SUPPORTS_MSI
    switch (entity->msi_support) {
        case PCI_ENTITY_MSI_SUPPORT_NONE:
            printk(LOGLEVEL_WARN,
                   "pcie: entity " PCI_ENTITY_INFO_FMT " does not support msi "
                   "or msix. failing to bind msi[x] to "
                   "vector " ISR_VECTOR_FMT "\n",
                   PCI_ENTITY_INFO_FMT_ARGS(entity),
                   vector);

            return -1;
        case PCI_ENTITY_MSI_SUPPORT_MSI: {
            spin_acquire_preempt_disable(&entity->lock);
            const uint64_t msi_address = isr_get_msi_address(cpu, vector);

            bind_msi_to_vector(entity, msi_address, vector, masked);
            spin_release_preempt_enable(&entity->lock);

            return 0;
        }
        case PCI_ENTITY_MSI_SUPPORT_MSIX: {
            spin_acquire_preempt_disable(&entity->lock);

            const uint64_t msix_address = isr_get_msix_address(cpu, vector);
            const uint16_t result =
                bind_msix_to_vector(entity, msix_address, vector, masked);

            spin_release_preempt_enable(&entity->lock);
            return result;
        }
    }
#else
    (void)entity;
    (void)cpu;
    (void)vector;
    (void)masked;
#endif /* defined(ISR_SUPPORTS_MSI) */

    verify_not_reached();
}

#if ISR_SUPPORTS_MSI
    __debug_optimize(3) static void
    toggle_msi_vector_mask(const struct pci_entity_info *const entity,
                           const isr_vector_t vector,
                           const bool masked)
    {
        if (!masked) {
            return;
        }

        if (entity->msi.supports_64bit) {
            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                bits64.mask_bits,
                                1ull << vector);
        } else {
            pci_write_from_base(entity,
                                entity->msi_pcie_offset,
                                struct pci_spec_cap_msi,
                                bits32.mask_bits,
                                1ull << vector);
        }
    }

    __debug_optimize(3) static void
    toggle_msix_vector_mask(const struct pci_entity_info *const entity,
                            const isr_vector_t vector,
                            const bool masked)
    {
        struct pci_entity_bar_info *const bar = entity->msix.table_bar;
        if (bar->mmio == NULL) {
            return;
        }

        volatile struct pci_spec_cap_msix_table_entry *const table =
            pci_entity_bar_get_base(bar) + entity->msix.table_offset;

        mmio_write(&table[vector].control, masked);
    }
#endif /* defined(ISR_SUPPORTS_MSI) */

bool
pci_entity_toggle_msi_vector_mask(struct pci_entity_info *const entity,
                                  const isr_vector_t vector,
                                  const bool mask)
{
#if ISR_SUPPORTS_MSI
    switch (entity->msi_support) {
        case PCI_ENTITY_MSI_SUPPORT_NONE:
            printk(LOGLEVEL_WARN,
                   "pcie: entity " PCI_ENTITY_INFO_FMT " does not support msi "
                   "or msix\n",
                   PCI_ENTITY_INFO_FMT_ARGS(entity));

            return false;
        case PCI_ENTITY_MSI_SUPPORT_MSI: {
            spin_acquire_preempt_disable(&entity->lock);

            toggle_msi_vector_mask(entity, vector, mask);
            spin_release_preempt_enable(&entity->lock);

            return true;
        }
        case PCI_ENTITY_MSI_SUPPORT_MSIX: {
            spin_acquire_preempt_disable(&entity->lock);

            toggle_msix_vector_mask(entity, vector, mask);
            spin_release_preempt_enable(&entity->lock);

            return true;
        }
    }
#else
    (void)entity;
    (void)vector;
    (void)mask;
#endif /* defined(ISR_SUPPORTS_MSI) */

    verify_not_reached();
}

__debug_optimize(3) void
pci_entity_enable_privls(struct pci_entity_info *const entity,
                         const uint16_t privls)
{
    spin_acquire_preempt_disable(&entity->lock);

    const uint16_t old_command =
        pci_read(entity, struct pci_spec_entity_info_base, command);
    const uint16_t new_command =
        (old_command | (privls & __PCI_ENTITY_PRIVL_MASK))
      ^ __PCI_DEVCMDREG_PIN_INTR_DISABLE;

    pci_write(entity, struct pci_spec_entity_info_base, command, new_command);
    spin_release_preempt_enable(&entity->lock);
}

__debug_optimize(3)
void pci_entity_disable_privls(struct pci_entity_info *const entity) {
    spin_acquire_preempt_disable(&entity->lock);

    const uint16_t old_command =
        pci_read(entity, struct pci_spec_entity_info_base, command);
    const uint16_t new_command =
        rm_mask(old_command, __PCI_ENTITY_PRIVL_MASK)
        | __PCI_DEVCMDREG_PIN_INTR_DISABLE;

    pci_write(entity, struct pci_spec_entity_info_base, command, new_command);
    spin_release_preempt_enable(&entity->lock);
}

__debug_optimize(3)
void pci_entity_info_destroy(struct pci_entity_info *const entity) {
    spinlock_deinit(&entity->lock);
    spin_acquire_preempt_disable(&entity->bus->lock);

    list_deinit(&entity->list_in_entities);
    list_deinit(&entity->list_in_domain);

    spin_release_preempt_enable(&entity->bus->lock);

    struct pci_entity_bar_info *const bar_list = entity->bar_list;
    const uint8_t bar_count =
        entity->header_kind == PCI_SPEC_ENTITY_HDR_KIND_PCI_BRIDGE  ?
            PCI_BAR_COUNT_FOR_BRIDGE : PCI_BAR_COUNT_FOR_GENERAL;

    for (uint8_t i = 0; i != bar_count; i++) {
        struct pci_entity_bar_info *const bar = &bar_list[i];
        if (bar->mmio != NULL) {
            vunmap_mmio(bar->mmio);
        }
    }

    kfree(entity->bar_list);
    entity->bar_list = NULL;

    switch (entity->msi_support) {
        case PCI_ENTITY_MSI_SUPPORT_NONE:
            break;
        case PCI_ENTITY_MSI_SUPPORT_MSI:
            entity->msi.supports_64bit = false;
            entity->msi.supports_masking = false;
            entity->msi.enabled = false;

            entity->msi_support = PCI_ENTITY_MSI_SUPPORT_NONE;
            break;
        case PCI_ENTITY_MSI_SUPPORT_MSIX:
            kfree(entity->msix.bitset);

            entity->msix.table_bar = NULL;
            entity->msix.bitset = NULL;
            entity->msix.table_offset = 0;
            entity->msix.table_size = 0;

            entity->msi_support = PCI_ENTITY_MSI_SUPPORT_NONE;
            break;
    }

    array_destroy(&entity->vendor_cap_list);

    entity->bus = NULL;
    entity->loc = PCI_LOCATION_NULL();

    entity->id = 0;
    entity->vendor_id = 0;
    entity->revision_id = 0;

    entity->prog_if = 0;
    entity->header_kind = 0;
    entity->subclass = 0;
    entity->class = 0;
    entity->subclass = 0;

    entity->interrupt_line = 0;
    entity->interrupt_pin = 0;

    entity->supports_pcie = false;
    entity->msix_enabled = false;

    entity->max_bar_count = 0;
    entity->msi_pcie_offset = 0;

    kfree(entity);
}
