/*
 * kernel/src/dev/nvme/init.c
 * © suhas pai
 */

#include "dev/pci/structs.h"

#include "asm/irqs.h"
#include "cpu/isr.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"
#include "mm/kmalloc.h"

#include "controller.h"

#define NVME_BAR_INDEX 0

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    if (!index_in_bounds(NVME_BAR_INDEX, pci_entity->max_bar_count)) {
        printk(LOGLEVEL_WARN, "nvme: pci-entity has no bars. aborting init\n");
        return;
    }

    if (pci_entity->msi_support == PCI_ENTITY_MSI_SUPPORT_NONE) {
        printk(LOGLEVEL_WARN,
               "nvme: pci-entity does not support msi[x]. aborting init\n");
        return;
    }

    printk(LOGLEVEL_INFO,
           "nvme: requester-id: 0x%" PRIx16 "\n",
           pci_entity_get_requester_id(pci_entity));

    struct pci_entity_bar_info *const bar =
        &pci_entity->bar_list[NVME_BAR_INDEX];

    if (!bar->is_present) {
        printk(LOGLEVEL_WARN,
               "nvme: pci-device doesn't have the required bar at "
               "index %" PRIu32 "\n",
               NVME_BAR_INDEX);
        return;
    }

    if (!bar->is_mmio) {
        printk(LOGLEVEL_WARN,
               "nvme: pci-device's bar at index %" PRIu32 " isn't an mmio "
               "bar\n",
               NVME_BAR_INDEX);
        return;
    }

    if (bar->port_or_phys_range.size < sizeof(struct nvme_registers)) {
        printk(LOGLEVEL_WARN,
               "nvme: pci-device's bar at index %" PRIu32 " memory range is "
               "too small for nvme registers\n",
               NVME_BAR_INDEX);
        return;
    }

    if (!pci_map_bar(bar)) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to map pci bar at index %" PRIu32 "\n",
               NVME_BAR_INDEX);
        return;
    }

    const isr_vector_t isr_vector =
        isr_alloc_msi_vector(&pci_entity->device, /*msi_index=*/0);

    if (isr_vector == ISR_INVALID_VECTOR) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to alloc isr vector for msix capability\n");
        return;
    }

    pci_entity_enable_privls(pci_entity,
                             __PCI_ENTITY_PRIVL_BUS_MASTER
                             | __PCI_ENTITY_PRIVL_MEM_ACCESS);

    const int flag = disable_irqs_if_enabled();
    if (!pci_entity_enable_msi(pci_entity)) {
        isr_free_msi_vector(&pci_entity->device, isr_vector, /*msi_index=*/0);
        printk(LOGLEVEL_WARN, "nvme: pci-entity is missing msi capability\n");

        return;
    }

    pci_entity_bind_msi_to_vector(pci_entity,
                                  this_cpu(),
                                  isr_vector,
                                  /*masked=*/false);

    enable_irqs_if_flag(flag);

    struct nvme_controller *const controller = kmalloc(sizeof(*controller));
    if (controller == NULL) {
        pci_entity_disable_msi(pci_entity);
        pci_entity_disable_privls(pci_entity);

        pci_unmap_bar(bar);

        isr_free_msi_vector(&pci_entity->device, isr_vector, /*msi_index=*/0);
        printk(LOGLEVEL_WARN, "nvme: failed to alloc memory\n");

        return;
    }

    volatile struct nvme_registers *const regs = pci_entity_bar_get_base(bar);
    if (!nvme_controller_create(controller,
                                &pci_entity->device,
                                regs,
                                isr_vector,
                                /*msix_vector=*/0))
    {
        kfree(controller);

        pci_entity_disable_msi(pci_entity);
        pci_entity_disable_privls(pci_entity);

        pci_unmap_bar(bar);
        isr_free_msi_vector(&pci_entity->device, isr_vector, /*msi_index=*/0);

        printk(LOGLEVEL_WARN, "nvme: failed to allocate queue\n");
        return;
    }
}

static const struct pci_driver pci_driver = {
    .class = PCI_ENTITY_CLASS_MASS_STORAGE_CONTROLLER,
    .subclass = PCI_ENTITY_SUBCLASS_NVME,
    .match = __PCI_DRIVER_MATCH_CLASS | __PCI_DRIVER_MATCH_SUBCLASS,
    .init = init_from_pci
};

__driver static const struct driver driver = {
    .name = SV_STATIC("nvme-driver"),
    .dtb = NULL,
    .pci = &pci_driver
};