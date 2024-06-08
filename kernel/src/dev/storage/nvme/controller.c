/*
 * kernel/src/dev/nvme/controller.c
 * © suhas pai
 */

#include "asm/pause.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "lib/util.h"
#include "lib/size.h"

#include "mm/kmalloc.h"
#include "mm/phalloc.h"

#include "sys/mmio.h"

#include "command.h"
#include "namespace.h"

// Recommended values from spec
#define NVME_SUBMIT_QUEUE_SIZE 6
#define NVME_COMPLETION_QUEUE_SIZE 4

#define NVME_QUEUE_PAGE_ALLOC_ORDER 0
#define NVME_VERSION(major, minor, tertiary) \
    ((uint32_t)major << NVME_VERSION_MAJOR_SHIFT \
     | (uint32_t)minor << NVME_VERSION_MINOR_SHIFT \
     | (uint32_t)tertiary)

static struct list g_controller_list = LIST_INIT(g_controller_list);
static uint32_t g_controller_count = 0;

static bool notify_queue_if_done(struct nvme_queue *const queue) {
    volatile struct nvme_completion_queue_entry *const completion_queue =
        queue->completion_queue_mmio->base;

    spin_acquire(&queue->lock);

    uint8_t head = queue->completion_queue_head;
    const uint32_t status = mmio_read(&completion_queue[head].status);

    if (status == 0) {
        spin_release(&queue->lock);
        return false;
    }

    const uint8_t phase = queue->phase;
    if ((status & __NVME_COMPL_QUEUE_ENTRY_STATUS_PHASE) != phase) {
        spin_release(&queue->lock);
        printk(LOGLEVEL_WARN,
               "nvme: queue with qid %" PRIu8 " has a phase mismatch error\n",
               queue->id);

        return false;
    }

    queue->completion_queue_head++;
    if (queue->completion_queue_head == queue->entry_count) {
        queue->completion_queue_head = 0;
        queue->phase = !queue->phase;
    }

    mmio_write(&queue->doorbells->complete, queue->completion_queue_head);

    event_trigger(&queue->event, /*drop_if_no_listeners=*/false);
    spin_release(&queue->lock);

    return true;
}

__optimize(3)
void handle_irq(const uint64_t int_no, struct thread_context *const context) {
    (void)context;

    struct nvme_controller *iter = NULL;
    bool found = false;

    list_foreach(iter, &g_controller_list, list) {
        if (iter->isr_vector == int_no) {
            found = true;
            break;
        }
    }

    if (!found) {
        isr_eoi(int_no);
        printk(LOGLEVEL_WARN,
               "nvme: got spurious interrupt from vector w/o corresponding "
               "controller: %" PRIu64 "\n",
               int_no);

        return;
    }

    if (notify_queue_if_done(&iter->admin_queue)) {
        isr_eoi(int_no);
        return;
    }

    // TODO: use GET_FEATURES command to check for interrupt coalescing

    struct nvme_namespace *ns_iter = NULL;
    list_foreach(ns_iter, &iter->namespace_list, list) {
        if (notify_queue_if_done(&ns_iter->io_queue)) {
            isr_eoi(int_no);
            return;
        }
    }

    isr_eoi(int_no);
    printk(LOGLEVEL_WARN,
           "nvme: got spurious interrupt w/o any corresponding queue\n");

    return;
}

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
        if (status & __NVME_CNTLR_STATUS_FATAL) {
            printk(LOGLEVEL_WARN, "nvme: fatal status in controller\n");
            return false;
        }

        if (status & __NVME_CNTLR_STATUS_READY) {
            return true;
        }

        cpu_pause();
    }

    printk(LOGLEVEL_WARN, "nvme: controller failed to get ready in time\n");
    return false;
}

static bool
identify_namespaces(struct nvme_controller *const controller,
                    const uint16_t max_queue_cmd_count)
{
    volatile struct nvme_registers *const regs = controller->regs;
    const uint64_t identity_phys = phalloc(sizeof(struct nvme_identity));

    if (identity_phys == INVALID_PHYS) {
        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");
        return false;
    }

    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_CONTROLLER,
                       identity_phys))
    {
        phalloc_free(identity_phys);
        printk(LOGLEVEL_WARN, "nvme: failed to alloc page for identity cmd\n");

        return false;
    }

    struct nvme_identity *const ident = phys_to_virt(identity_phys);

    uint32_t max_transfer_shift = 0;
    const uint32_t max_data_transfer_shift = ident->max_data_transfer_shift;

    if (max_data_transfer_shift != 0) {
        max_transfer_shift =
            ((regs->capabilities & __NVME_CAP_MEM_PAGE_SIZE_MIN_4KiB) >>
                NVME_CAP_MEM_PAGE_SIZE_MIN_SHIFT) + 12;
    } else {
        max_transfer_shift = 20;
    }

    const uint32_t namespace_count = ident->namespace_count;
    printk(LOGLEVEL_INFO,
           "nvme identity:\n"
           "\tvendor-id: 0x%" PRIx16 "\n"
           "\tsubsystem vendor-id: 0x%" PRIx16 "\n"
           "\tserial number: " SV_FMT "\n"
           "\tmodel number: " SV_FMT "\n"
           "\tfirmare revision: " SV_FMT "\n"
           "\tnamespace count: %" PRIu32 "\n",
           ident->vendor_id,
           ident->subsystem_vendor_id,
           SV_FMT_ARGS(sv_of_carr(ident->serial_number)),
           SV_FMT_ARGS(sv_of_carr(ident->model_number)),
           SV_FMT_ARGS(sv_of_carr(ident->firmware_revision)),
           namespace_count);

    if (!nvme_identify(controller,
                       /*nsid=*/0,
                       NVME_IDENTIFY_CNS_ACTIVE_NSID_LIST,
                       identity_phys))
    {
        phalloc_free(identity_phys);
        printk(LOGLEVEL_WARN, "nvme: failed to identify controller\n");

        return false;
    }

    if (!nvme_set_number_of_queues(controller, /*queue_count=*/4)) {
        phalloc_free(identity_phys);
        printk(LOGLEVEL_WARN, "nvme: failed to set number of queues\n");

        return false;
    }

    const uint32_t *const nsid_list = (const uint32_t *)(uint64_t)ident;
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

        printk(LOGLEVEL_INFO,
               "nvme: initialized namespace %" PRIu32 "\n",
               namespace->nsid);
    }

    phalloc_free(identity_phys);
    return true;
}

bool
nvme_controller_create(struct nvme_controller *const controller,
                       struct device *const device,
                       volatile struct nvme_registers *const regs,
                       const isr_vector_t isr_vector,
                       const uint16_t msix_vector)
{
    list_init(&controller->list);
    list_init(&controller->namespace_list);

    controller->device = device;
    controller->regs = regs;
    controller->lock = SPINLOCK_INIT();
    controller->msix_vector = msix_vector;
    controller->isr_vector = isr_vector;

    if (!nvme_queue_create(&controller->admin_queue,
                           controller,
                           /*id=*/0,
                           NVME_ADMIN_QUEUE_COUNT,
                           /*max_transfer_shift=*/0))
    {
        return false;
    }

    const uint32_t capabilities = mmio_read(&regs->capabilities);
    const uint16_t max_queue_cmd_count =
        (capabilities & __NVME_CAP_MAX_QUEUE_ENTRIES) + 1;

    printk(LOGLEVEL_INFO,
           "nvme: supports upto %" PRIu16 " queues\n",
           max_queue_cmd_count);

    const uint8_t stride_offset =
        (capabilities & __NVME_CAP_DOORBELL_STRIDE) >>
            NVME_CAP_DOORBELL_STRIDE_SHIFT;

    printk(LOGLEVEL_INFO,
           "nvme: stride is " SIZE_UNIT_FMT "\n",
           SIZE_UNIT_FMT_ARGS_ABBREV(controller->stride));

    controller->stride = 2ull << (2 + stride_offset);
    const uint32_t version = mmio_read(&regs->version);

    printk(LOGLEVEL_WARN,
           "nvme: version is " NVME_VERSION_FMT "\n",
           NVME_VERSION_FMT_ARGS(version));

    if (version < NVME_VERSION(1, 4, 0)) {
        printk(LOGLEVEL_WARN, "nvme: version too old, unsupported\n");
        return false;
    }

    const uint32_t config = mmio_read(&regs->config);
    if (config & __NVME_CONFIG_ENABLE) {
        mmio_write(&regs->config, rm_mask(config, __NVME_CONFIG_ENABLE));
    }

    if (!wait_until_stopped(regs)) {
        printk(LOGLEVEL_WARN, "nvme: controller failed to stop in time\n");
        return false;
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
        printk(LOGLEVEL_WARN, "nvme: controller failed to restart in time\n");
        return false;
    }

    printk(LOGLEVEL_INFO,
           "nvme: binding isr msix vector " ISR_VECTOR_FMT " to msix "
           "table index %" PRIu16 "\n",
           controller->isr_vector,
           controller->msix_vector);

    isr_set_msi_vector(controller->isr_vector,
                       handle_irq,
                       &ARCH_ISR_INFO_NONE());

    list_add(&g_controller_list, &controller->list);
    g_controller_count++;

    if (!identify_namespaces(controller, max_queue_cmd_count)) {
        mmio_write(&regs->config, 0);

        list_remove(&controller->list);
        g_controller_count--;

        return false;
    }

    printk(LOGLEVEL_INFO, "nvme: finished init\n");
    return true;
}

bool nvme_controller_destroy(struct nvme_controller *const controller) {
    spinlock_deinit(&controller->lock);

    struct nvme_namespace *iter = NULL;
    list_foreach(iter, &controller->namespace_list, list) {
        nvme_namespace_destroy(iter);
    }

    isr_free_msi_vector(controller->device,
                        controller->isr_vector,
                        /*msi_index=*/0);

    list_deinit(&controller->list);
    list_deinit(&controller->namespace_list);

    controller->regs = NULL;
    controller->stride = 0;
    controller->msix_vector = 0;
    controller->isr_vector = 0;

    kfree(controller);
    return true;
}
