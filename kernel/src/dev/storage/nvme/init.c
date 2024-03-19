/*
 * kernel/src/dev/nvme/init.c
 * © suhas pai
 */

#include "dev/pci/structs.h"
#include "asm/pause.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/size.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/phalloc.h"

#include "sys/mmio.h"
#include "namespace.h"

#define NVME_BAR_INDEX 0

// Recommended values from spec
#define NVME_SUBMIT_QUEUE_SIZE 6
#define NVME_COMPLETION_QUEUE_SIZE 4

#define NVME_QUEUE_PAGE_ALLOC_ORDER 0

#define NVME_VERSION(major, minor, tertiary) \
    ((uint32_t)major << NVME_VERSION_MAJOR_SHIFT \
     | (uint32_t)minor << NVME_VERSION_MINOR_SHIFT \
     | (uint32_t)tertiary)

#define MAX_ATTEMPTS 100

__optimize(3) static inline
bool wait_until_stopped(volatile struct nvme_registers *const regs) {
    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&regs->status) & __NVME_CNTLR_STATUS_READY) == 0) {
            return true;
        }

        cpu_pause();
    }

    return false;
}

__optimize(3) static inline
bool wait_until_ready(volatile struct nvme_registers *const regs) {
    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        const uint8_t status = mmio_read(&regs->status);
        if (status & __NVME_CNTLR_STATUS_READY) {
            return true;
        }

        if (status & __NVME_CNTLR_STATUS_FATAL) {
            printk(LOGLEVEL_WARN, "nvme: fatal status in controller\n");
            return false;
        }

        cpu_pause();
    }

    printk(LOGLEVEL_WARN, "nvme: controller failed to get ready in time\n");
    return false;
}

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    if (!index_in_bounds(NVME_BAR_INDEX, pci_entity->max_bar_count)) {
        printk(LOGLEVEL_WARN, "nvme: pci-entity has no bars\n");
        return;
    }

    if (pci_entity->msi_support == PCI_ENTITY_MSI_SUPPORT_NONE) {
        printk(LOGLEVEL_WARN, "nvme: pci-entity does not support msi[x]\n");
        return;
    }

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

    // Don't enable interrupts priviledge, as it is separate from MSI[X] which
    // we'll be using.
    pci_entity_enable_privl(pci_entity,
                            __PCI_ENTITY_PRIVL_BUS_MASTER
                            | __PCI_ENTITY_PRIVL_MEM_ACCESS);

    volatile struct nvme_registers *const regs =
        (volatile struct nvme_registers *)bar->mmio->base;

    const uint32_t capabilities = mmio_read(&regs->capabilities);
    const uint16_t max_queue_cmd_count =
        (capabilities & __NVME_CAP_MAX_QUEUE_ENTRIES) + 1;

    printk(LOGLEVEL_INFO,
           "nvme: supports upto %" PRIu16 " queues\n",
           max_queue_cmd_count);

    const uint8_t stride_offset =
        (capabilities & __NVME_CAP_DOORBELL_STRIDE) >>
            NVME_CAP_DOORBELL_STRIDE_SHIFT;

    const uint32_t stride = 2ull << (2 + stride_offset);
    printk(LOGLEVEL_INFO,
           "nvme: stride is " SIZE_UNIT_FMT "\n",
           SIZE_UNIT_FMT_ARGS_ABBREV(stride));

    const uint32_t version = mmio_read(&regs->version);
    printk(LOGLEVEL_WARN,
           "nvme: version is " NVME_VERSION_FMT "\n",
           NVME_VERSION_FMT_ARGS(version));

    if (version < NVME_VERSION(1, 4, 0)) {
        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: version too old, unsupported\n");
        return;
    }

    struct nvme_controller *const controller = kmalloc(sizeof(*controller));
    if (controller == NULL) {
        kfree(controller);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc memory\n");
        return;
    }

    if (!nvme_controller_create(controller,
                                pci_entity,
                                regs,
                                stride,
                                /*msix_vector=*/0))
    {
        kfree(controller);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to allocate queue\n");
        return;
    }

    const uint32_t config = mmio_read(&regs->config);
    if (config & __NVME_CONFIG_ENABLE) {
        mmio_write(&regs->config, rm_mask(config, __NVME_CONFIG_ENABLE));
    }

    if (!wait_until_stopped(regs)) {
        nvme_controller_destroy(controller);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: controller failed to stop in time\n");
        return;
    }

    mmio_write(&regs->admin_queue_attributes,
               (NVME_ADMIN_QUEUE_COUNT - 1) <<
                NVME_ADMIN_QUEUE_ATTR_SUBMIT_QUEUE_SIZE_SHIFT
               | (NVME_ADMIN_QUEUE_COUNT - 1));

    mmio_write(&regs->admin_submit_queue_base_addr,
               controller->admin_queue.submit_queue_phys);
    mmio_write(&regs->admin_completion_queue_base_addr,
               controller->admin_queue.completion_queue_phys);

    mmio_write(&regs->config,
               NVME_SUBMIT_QUEUE_SIZE <<
                NVME_CONFIG_IO_SUBMIT_QUEUE_ENTRY_SIZE_SHIFT
               | NVME_COMPLETION_QUEUE_SIZE <<
                NVME_CONFIG_IO_COMPL_QUEUE_ENTRY_SIZE_SHIFT
               | __NVME_CONFIG_ENABLE);

    if (!wait_until_ready(regs)) {
        nvme_controller_destroy(controller);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        return;
    }

    printk(LOGLEVEL_INFO,
           "nvme: binding vector " ISR_VECTOR_FMT " to msix "
           "vector %" PRIu16 "\n",
           controller->isr_vector,
           controller->msix_vector);

    const uint64_t identity_phys =
        phalloc(sizeof(struct nvme_controller_identity));

    if (identity_phys == INVALID_PHYS) {
        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return;
    }

    struct nvme_controller_identity *const identity =
        phys_to_virt(identity_phys);

    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_CONTROLLER,
                       identity_phys))
    {
        phalloc_free(identity_phys);

        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return;
    }

    uint32_t max_transfer_shift = 0;
    const uint32_t max_data_transfer_shift = identity->max_data_transfer_shift;

    if (max_data_transfer_shift != 0) {
        max_transfer_shift =
            ((regs->capabilities & __NVME_CAP_MEM_PAGE_SIZE_MIN_4KiB) >>
                NVME_CAP_MEM_PAGE_SIZE_MIN_SHIFT) + 12;
    } else {
        max_transfer_shift = 20;
    }

    const uint32_t namespace_count = identity->namespace_count;
    printk(LOGLEVEL_INFO,
           "nvme identity:\n"
           "\tvendor-id: 0x%" PRIx16 "\n"
           "\tsubsystem vendor-id: 0x%" PRIx16 "\n"
           "\tserial number: " SV_FMT "\n"
           "\tmodel number: " SV_FMT "\n"
           "\tfirmare revision: " SV_FMT "\n"
           "\tnamespace count: %" PRIu32 "\n",
           identity->vendor_id,
           identity->subsystem_vendor_id,
           SV_FMT_ARGS(sv_of_carr(identity->serial_number)),
           SV_FMT_ARGS(sv_of_carr(identity->model_number)),
           SV_FMT_ARGS(sv_of_carr(identity->firmware_revision)),
           namespace_count);

    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_ACTIVE_NSID_LIST,
                       identity_phys))
    {
        phalloc_free(identity_phys);

        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to identify controller\n");
        return;
    }

    nvme_set_number_of_queues(controller, /*queue_count=*/4);

    const uint32_t *const nsid_list = (const uint32_t *)(uint64_t)identity;
    for (uint32_t i = 0; i != namespace_count; i++) {
        const uint32_t nsid = nsid_list[i];
        if (!ordinal_in_bounds(nsid, namespace_count)) {
            continue;
        }

        struct nvme_namespace *const namespace = kmalloc(sizeof(*namespace));
        if (namespace == NULL) {
            continue;
        }

        if (!nvme_namespace_create(namespace,
                                   controller,
                                   nsid,
                                   max_queue_cmd_count,
                                   max_transfer_shift))
        {
            kfree(namespace);
            continue;
        }

        if (!storage_device_create(&namespace->device,
                                   namespace->lba_size,
                                   nvme_read,
                                   nvme_write))
        {
            nvme_namespace_destroy(namespace);
            continue;
        }

        printk(LOGLEVEL_INFO,
               "nvme: initialized namespace %" PRIu32 "\n",
               namespace->nsid);
    }

    phalloc_free(identity_phys);
    printk(LOGLEVEL_INFO, "nvme: finished init\n");
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