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
#include "device.h"

#define NVME_BAR_INDEX 0
#define NVME_ADMIN_QUEUE_COUNT 32
#define NVME_IO_QUEUE_COUNT 1024ul

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

__optimize(3) bool
nvme_identify(struct nvme_controller *const controller,
              const uint32_t nsid,
              const enum nvme_identify_cns cns,
              void *const out)
{
    struct nvme_command command = NVME_IDENTIFY_CMD(nsid, cns, out);
    return nvme_queue_submit_command(&controller->admin_queue, &command);
}

static bool
nvme_add_namespace(struct nvme_controller *const controller,
                   const uint32_t nsid,
                   const uint32_t max_queue_count,
                   const uint32_t max_transfer_shift)
{
    struct nvme_namespace *const namespace = kmalloc(sizeof(*namespace));
    if (namespace == NULL) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to allocate namespace-info struct\n");
        return false;
    }

    struct page *const page = alloc_page(PAGE_STATE_USED, /*alloc_flags=*/0);
    if (page == NULL) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to alloc page for identify-namespace command\n");
        return false;
    }

    struct nvme_namespace_identity *const identity = page_to_virt(page);
    if (!nvme_identify(controller, nsid, NVME_IDENTIFY_CNS_NAMESPACE, identity))
    {
        free_page(page);
        printk(LOGLEVEL_WARN,
               "nvme: identify-namespace command failed for nsid %" PRIu32 "\n",
               nsid);

        return false;
    }

    const uint32_t formatted_lba = identity->formatted_lba_count & 0xF;
    const struct nvme_lba lba = identity->lbaf[formatted_lba];
    const uint32_t lba_size = 1ull << lba.lba_data_size_shift;

    printk(LOGLEVEL_INFO,
           "nvme: namespace (nsid=%" PRIu32 ") identity\n"
           "\tsize: %" PRIu64 " blocks\n"
           "\t\ttotal size: " SIZE_UNIT_FMT "\n"
           "\tcapacity: %" PRIu64 " blocks\n"
           "\t\ttotal capacity: " SIZE_UNIT_FMT "\n"
           "\tutilization: %" PRIu64 " blocks\n"
           "\t\ttotal utilization: " SIZE_UNIT_FMT "\n"
           "\tfeatures: 0x%" PRIx8 "\n"
           "\tformatted lba count: %" PRIu8 "\n",
           nsid,
           identity->size,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->size * lba_size),
           identity->capacity,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->capacity * lba_size),
           identity->utilization,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->utilization * lba_size),
           identity->features,
           identity->formatted_lba_count);

    printk(LOGLEVEL_INFO,
            "\t\tlba %" PRIu32 ":\n"
            "\t\t\tmetadata size: " SIZE_UNIT_FMT "\n"
            "\t\t\tlba data size shift: %" PRIu8 "\n"
            "\t\t\trelative performance: %" PRIu8 "\n",
            formatted_lba + 1,
            SIZE_UNIT_FMT_ARGS_ABBREV(lba.metadata_size),
            lba.lba_data_size_shift,
            lba.relative_performance);

    printk(LOGLEVEL_INFO,
           "\tformatted lba size: %" PRIu8 "\n"
           "\tmetadata capabilities: 0x%" PRIu8 "\n"
           "\tdata protection capabilities: 0x%" PRIu8 "\n"
           "\tdata protection settings: 0x%" PRIu8 "\n"
           "\tnmic: 0x%" PRIu8 "\n"
           "\treservation capabilities: 0x%" PRIu8 "\n"
           "\tformat progress indicator: 0x%" PRIu8 "\n"
           "\tdealloc lba features: 0x%" PRIu8 "\n"
           "\tnamespace atomic write unit normal: %" PRIu16 "\n"
           "\tnamespace atomic write unit power fail: %" PRIu16 "\n"
           "\tnamespace compare and write unit: %" PRIu16 "\n"
           "\tnamespace boundary size normal: %" PRIu16 "\n"
           "\tnamespace boundary size power fail: %" PRIu16 "\n"
           "\tnamespace optimal i/o boundary: %" PRIu16 "\n"
           "\tnvm capability lower: 0x%" PRIu64 "\n"
           "\tnvm capability upper: 0x%" PRIu64 "\n"
           "\tnvm preferred write granularity: %" PRIu16 "\n"
           "\tnvm preferred write alignment: %" PRIu16 "\n"
           "\tnvm preferred dealloc granularity: %" PRIu16 "\n"
           "\tnvm preferred dealloc alignment: %" PRIu16 "\n"
           "\tnamespace optimal write size: %" PRIu16 "\n"
           "\tmax single source range length: %" PRIu16 "\n"
           "\tmax copy length: %" PRIu32 "\n"
           "\tmin source range count: %" PRIu8 "\n"
           "\tattributes: %" PRIu8 "\n"
           "\tnvm set-id: %" PRIu16 "\n"
           "\tiee uid: %" PRIu64 "\n",
           identity->formatted_lba_index,
           identity->metadata_capabilities,
           identity->data_protection_capabilities,
           identity->data_protection_settings,
           identity->nmic,
           identity->reservation_capabilities,
           identity->format_progress_indicator,
           identity->dealloc_lba_features,
           identity->namespace_atomic_write_unit_normal,
           identity->namespace_atomic_write_unit_power_fail,
           identity->namespace_cmp_and_write_unit,
           identity->namespace_boundary_size_normal,
           identity->namespace_boundary_size_power_fail,
           identity->namespace_optimal_io_boundary,
           identity->nvm_capability_lower64,
           identity->nvm_capability_upper64,
           identity->namespace_pref_write_gran,
           identity->namespace_pref_write_align,
           identity->namespace_pref_dealloc_gran,
           identity->namespace_pref_dealloc_align,
           identity->namespace_optim_write_size,
           identity->max_single_source_range_length,
           identity->max_copy_length,
           identity->min_source_range_count,
           identity->attributes,
           identity->nvm_set_id,
           identity->iee_uid);

    struct nvme_queue *const queue = &namespace->queue;
    const uint16_t io_queue_entry_count =
        min(NVME_IO_QUEUE_COUNT, max_queue_count);

    if (!nvme_queue_create(queue,
                           controller,
                           nsid,
                           io_queue_entry_count,
                           max_transfer_shift))
    {
        return false;
    }

    struct nvme_command comp_command =
        NVME_CREATE_COMPLETION_QUEUE_CMD(queue, controller->vector);

    if (!nvme_queue_submit_command(&controller->admin_queue, &comp_command)) {
        return false;
    }

    struct nvme_command submit_command = NVME_CREATE_SUBMIT_QUEUE_CMD(queue);
    if (!nvme_queue_submit_command(&controller->admin_queue, &submit_command)) {
        return false;
    }

    namespace->controller = controller;
    namespace->nsid = nsid;

    free_page(page);
    return true;
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

    list_init(&controller->list);

    controller->regs = regs;
    controller->stride = stride;

    if (!nvme_queue_create(&controller->admin_queue,
                           controller,
                           /*id=*/0,
                           NVME_ADMIN_QUEUE_COUNT,
                           /*max_transfer_shift=*/0))
    {
        kfree(controller);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: version too old, unsupported\n");
        return;
    }

    const uint32_t config = mmio_read(&regs->config);
    if (config & __NVME_CONFIG_ENABLE) {
        mmio_write(&regs->config, rm_mask(config, __NVME_CONFIG_ENABLE));
    }

    if (!wait_until_stopped(regs)) {
        kfree(controller);

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
        nvme_queue_destroy(&controller->admin_queue);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        return;
    }

    controller->vector = isr_alloc_vector(/*for_msi=*/true);
    if (controller->vector == ISR_INVALID_VECTOR) {
        kfree(controller);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to allocate queue\n");
        return;
    }

    printk(LOGLEVEL_INFO,
           "nvme: binding vector " ISR_VECTOR_FMT " for msix\n",
           controller->vector);

    const int flag = disable_interrupts_if_not();

    pci_entity_enable_msi(pci_entity);
    pci_entity_bind_msi_to_vector(pci_entity,
                                  this_cpu(),
                                  controller->vector,
                                  /*masked=*/false);

    enable_interrupts_if_flag(flag);

    struct page *const page = alloc_page(PAGE_STATE_USED, /*flags=*/0);
    if (page == NULL) {
        pci_entity_disable_msi(pci_entity);
        nvme_queue_destroy(&controller->admin_queue);

        kfree(controller);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return;
    }

    struct nvme_controller_identity *const identity = page_to_virt(page);
    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_CONTROLLER,
                       identity))
    {
        free_page(page);

        pci_entity_disable_msi(pci_entity);
        nvme_queue_destroy(&controller->admin_queue);

        kfree(controller);
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
                       identity))
    {
        free_page(page);

        pci_entity_disable_msi(pci_entity);
        nvme_queue_destroy(&controller->admin_queue);

        kfree(controller);
        mmio_write(&regs->config, 0);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);

        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return;
    }

    const uint32_t *const nsid_list = (const uint32_t *)(uint64_t)identity;
    for (uint32_t i = 0; i != namespace_count; i++) {
        const uint32_t nsid = nsid_list[i];
        if (!ordinal_in_bounds(nsid, namespace_count)) {
            continue;
        }

        if (!nvme_add_namespace(controller,
                                nsid,
                                max_queue_cmd_count,
                                max_transfer_shift))
        {
            free_page(page);

            pci_entity_disable_msi(pci_entity);
            nvme_queue_destroy(&controller->admin_queue);

            kfree(controller);
            mmio_write(&regs->config, 0);

            pci_entity_disable_privls(pci_entity);
            pci_unmap_bar(bar);

            printk(LOGLEVEL_WARN,
                   "nvme: failed to alloc page for identity cmd\n");
            return;
        }
    }

    g_controller_count++;
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