/*
 * kernel/src/dev/nvme/controller.c
 * © suhas pai
 */

#include "dev/pci/entity.h"

#include "asm/irqs.h"
#include "cpu/isr.h"
#include "dev/printk.h"
#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "command.h"
#include "namespace.h"

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
void handle_irq(const uint64_t int_no, struct thread_context *const frame) {
    (void)frame;

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

bool
nvme_controller_create(struct nvme_controller *const controller,
                       struct pci_entity_info *pci_entity,
                       volatile struct nvme_registers *const regs,
                       const uint8_t stride,
                       const uint16_t msix_vector)
{
    list_init(&controller->list);
    list_init(&controller->namespace_list);

    controller->pci_entity = pci_entity;
    controller->regs = regs;
    controller->stride = stride;
    controller->lock = SPINLOCK_INIT();
    controller->msix_vector = msix_vector;
    controller->isr_vector = isr_alloc_vector(/*for_msi=*/true);

    if (controller->isr_vector == ISR_INVALID_VECTOR) {
        return false;
    }

    isr_set_vector(controller->isr_vector, handle_irq, &ARCH_ISR_INFO_NONE());

    const int flag = disable_interrupts_if_not();
    if (!pci_entity_enable_msi(pci_entity)) {
        isr_free_vector(controller->isr_vector, /*for_msi=*/true);
        printk(LOGLEVEL_WARN, "nvme: pci-entity is missing msi capability\n");

        return false;
    }

    pci_entity_bind_msi_to_vector(pci_entity,
                                  this_cpu(),
                                  controller->isr_vector,
                                  /*masked=*/false);

    enable_interrupts_if_flag(flag);
    if (!nvme_queue_create(&controller->admin_queue,
                           controller,
                           /*id=*/0,
                           NVME_ADMIN_QUEUE_COUNT,
                           /*max_transfer_shift=*/0))
    {
        pci_entity_disable_msi(pci_entity);
        isr_free_vector(controller->isr_vector, /*for_msi=*/true);

        return false;
    }

    list_add(&g_controller_list, &controller->list);
    g_controller_count++;

    return true;
}

bool
nvme_identify(struct nvme_controller *const controller,
              const uint32_t nsid,
              const enum nvme_identify_cns cns,
              const uint64_t out)
{
    const struct nvme_command command =
        NVME_IDENTIFY_CMD(&controller->admin_queue, nsid, cns, out);

    return nvme_queue_submit_command(&controller->admin_queue, &command);
}

bool
nvme_create_submit_queue(struct nvme_controller *const controller,
                         const struct nvme_namespace *const namespace)
{
    const struct nvme_command submit_command =
        NVME_CREATE_SUBMIT_QUEUE_CMD(controller, &namespace->io_queue);

    return nvme_queue_submit_command(&controller->admin_queue, &submit_command);
}

bool
nvme_create_completion_queue(struct nvme_controller *const controller,
                             const struct nvme_namespace *const namespace)
{
    const struct nvme_command comp_command =
        NVME_CREATE_COMPLETION_QUEUE_CMD(controller,
                                         &namespace->io_queue,
                                         controller->msix_vector);

    return nvme_queue_submit_command(&controller->admin_queue, &comp_command);
}

bool
nvme_set_number_of_queues(struct nvme_controller *const controller,
                          const uint16_t queue_count)
{
    const struct nvme_command setft_command =
        NVME_SET_FEATURES_CMD(NVME_CMD_FEATURE_QUEUE_COUNT,
                              (uint32_t)queue_count << 16 | queue_count,
                              /*prp1=*/0,
                              /*prp2=*/0);

    return nvme_queue_submit_command(&controller->admin_queue, &setft_command);
}

bool nvme_controller_destroy(struct nvme_controller *const controller) {
    spinlock_deinit(&controller->lock);

    struct nvme_namespace *iter = NULL;
    list_foreach(iter, &controller->namespace_list, list) {
        nvme_namespace_destroy(iter);
    }

    isr_free_vector(controller->isr_vector, /*for_msi=*/true);

    list_deinit(&controller->list);
    list_deinit(&controller->namespace_list);

    controller->pci_entity = NULL;
    controller->regs = NULL;
    controller->stride = 0;
    controller->msix_vector = 0;
    controller->isr_vector = 0;

    kfree(controller);
    return true;
}
