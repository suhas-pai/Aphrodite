/*
 * kernel/src/dev/nvme/init.c
 * © suhas pai
 */

#include "dev/pci/structs.h"

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cpu/isr.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/size.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/page_alloc.h"

#include "sys/mmio.h"

#include "controller.h"
#include "namespace.h"

#define NVME_BAR_INDEX 0

// Recommended values from spec
#define NVME_SUBMIT_QUEUE_SIZE 6
#define NVME_COMPLETION_QUEUE_SIZE 4

#define NVME_QUEUE_PAGE_ALLOC_ORDER 0

#define NVME_VERSION(major, minor, tertiary) \
    ((uint32_t)major << NVME_VERSION_MAJOR_SHIFT | \
     (uint32_t)minor << NVME_VERSION_MINOR_SHIFT | \
     (uint32_t)tertiary)

static struct list g_controller_list = LIST_INIT(g_controller_list);
static uint32_t g_controller_count = 0;

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

__optimize(3)
void handle_irq(const uint64_t int_no, struct thread_context *const frame) {
    (void)frame;

    printk(LOGLEVEL_INFO, "nvme: got irq\n");
    isr_eoi(int_no);
}

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    if (!index_in_bounds(NVME_BAR_INDEX, pci_entity->max_bar_count)) {
        printk(LOGLEVEL_WARN, "nvme: pci-entity has no bars\n");
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

    pci_entity_enable_privl(pci_entity,
                            __PCI_ENTITY_PRIVL_BUS_MASTER |
                            __PCI_ENTITY_PRIVL_MEM_ACCESS |
                            __PCI_ENTITY_PRIVL_INTERRUPTS);

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

    if (!nvme_controller_create(controller, regs, stride)) {
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
                NVME_ADMIN_QUEUE_ATTR_SUBMIT_QUEUE_SIZE_SHIFT |
               (NVME_ADMIN_QUEUE_COUNT - 1));

    mmio_write(&regs->admin_submit_queue_base_addr,
               page_to_phys(controller->admin_queue.submit_queue_pages));
    mmio_write(&regs->admin_completion_queue_base_addr,
               page_to_phys(controller->admin_queue.completion_queue_pages));

    mmio_write(&regs->config,
               NVME_SUBMIT_QUEUE_SIZE <<
                NVME_CONFIG_IO_SUBMIT_QUEUE_ENTRY_SIZE_SHIFT |
               NVME_COMPLETION_QUEUE_SIZE <<
                NVME_CONFIG_IO_COMPL_QUEUE_ENTRY_SIZE_SHIFT |
               __NVME_CONFIG_ENABLE);

    if (!wait_until_ready(regs)) {
        nvme_controller_destroy(controller);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        return;
    }

    isr_set_vector(controller->vector, handle_irq, &ARCH_ISR_INFO_NONE());
    printk(LOGLEVEL_INFO,
           "nvme: binding vector " ISR_VECTOR_FMT " for msix\n",
           controller->vector);

    const int flag = disable_interrupts_if_not();
    if (!pci_entity_enable_msi(pci_entity)) {
        nvme_controller_destroy(controller);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: pci-entity is missing msi capability\n");
        return;
    }

    pci_entity_bind_msi_to_vector(pci_entity,
                                  this_cpu(),
                                  controller->vector,
                                  /*masked=*/false);

    enable_interrupts_if_flag(flag);

    struct page *const page = alloc_page(PAGE_STATE_USED, /*flags=*/0);
    if (page == NULL) {
        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return;
    }

    const uint64_t phys = page_to_phys(page);
    struct nvme_controller_identity *const identity = phys_to_virt(phys);

    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_CONTROLLER,
                       phys))
    {
        free_page(page);

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
                       phys))
    {
        free_page(page);

        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to identify controller\n");
        return;
    }

    nvme_set_number_of_queues(controller, /*queue_count=*/4);
    struct hashmap hashmap =
        HASHMAP_INIT(sizeof(struct nvme_namespace *),
                     /*bucket_count=*/10,
                     hashmap_no_hash,
                     NULL);

    struct page *const first_sectors_page =
        alloc_page(PAGE_STATE_USED, __ALLOC_ZERO);

    if (first_sectors_page == NULL) {
        free_page(page);

        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to identify controller\n");
        return;
    }

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

        if (!hashmap_add(&hashmap, hashmap_key_create(nsid), &namespace)) {
            kfree(namespace);
            continue;
        }

        if (!nvme_namespace_create(namespace,
                                   controller,
                                   nsid,
                                   max_queue_cmd_count,
                                   max_transfer_shift))
        {
            hashmap_remove(&hashmap, hashmap_key_create(nsid), /*object=*/NULL);
            kfree(namespace);

            continue;
        }

        const uint64_t first_sectors_phys = page_to_phys(first_sectors_page);
        if (!nvme_namespace_rwlba(namespace,
                                  RANGE_INIT(0, 1),
                                  /*write=*/false,
                                  first_sectors_phys))
        {
            hashmap_remove(&hashmap, hashmap_key_create(nsid), /*object=*/NULL);
            nvme_namespace_destroy(namespace);

            continue;
        }

        const void *const first_sectors = phys_to_virt(first_sectors_phys);
        printk(LOGLEVEL_INFO,
               "nvme: string is: " SV_FMT "\n",
               SV_FMT_ARGS(sv_create_length(first_sectors + 512, 8)));

        printk(LOGLEVEL_INFO, "nvme: rwlba success\n");
    }

    free_page(page);
    free_page(first_sectors_page);

    uint32_t count = 0;
    hashmap_foreach_node(&hashmap, iter) {
        struct nvme_namespace **const namespace = (void *)(uint64_t)iter->data;
        list_add(&controller->namespace_list, &(*namespace)->list);

        count++;
    }

    hashmap_destroy(&hashmap);
    if (count == 0) {
        pci_entity_disable_msi(pci_entity);
        nvme_controller_destroy(controller);

        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN,
               "nvme: no namespaces were found. aborting init\n");

        return;
    }

    g_controller_count++;
    printk(LOGLEVEL_INFO,
           "nvme: finished init, controller has %" PRIu32 " namespace(s)\n",
           count);
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