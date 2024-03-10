
/*
 * kernel/src/dev/nvme/queue.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "mm/mmio.h"
#include "mm/page_alloc.h"

#include "sys/mmio.h"
#include "device.h"

bool
nvme_queue_create(struct nvme_queue *const queue,
                  struct nvme_controller *const device,
                  const uint8_t id,
                  const uint16_t entry_count,
                  const uint8_t max_transfer_shift)
{
    const uint32_t submit_alloc_size =
        sizeof(struct nvme_command) * entry_count;

    uint8_t submit_order = 0;
    while ((PAGE_SIZE << submit_order) < submit_alloc_size) {
        submit_order++;
    }

    struct page *const submit_queue_pages =
        alloc_pages(PAGE_STATE_USED, submit_order, /*alloc_flags=*/0);

    if (submit_queue_pages == NULL) {
        printk(LOGLEVEL_WARN, "nvme: failed to allocate page for nvme-queue\n");
        return false;
    }

    const uint32_t completion_alloc_size =
        sizeof(struct nvme_completion_queue_entry) * entry_count;

    uint8_t completion_order = 0;
    while ((PAGE_SIZE << completion_order) < completion_alloc_size) {
        completion_order++;
    }

    struct page *const completion_queue_pages =
        alloc_pages(PAGE_STATE_USED, completion_order, /*alloc_flags=*/0);

    if (completion_queue_pages == NULL) {
        free_pages(submit_queue_pages, submit_order);
        printk(LOGLEVEL_WARN, "nvme: failed to allocate page for nvme-queue\n");

        return false;
    }

    struct mmio_region *const submit_queue_mmio =
        vmap_mmio(RANGE_INIT(page_to_phys(submit_queue_pages),
                             PAGE_SIZE << submit_order),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (submit_queue_mmio == NULL) {
        free_pages(submit_queue_pages, submit_order);
        free_pages(completion_queue_pages, completion_order);

        printk(LOGLEVEL_WARN,
               "nvme: failed to vmap submit-queue page for nvme-queue\n");
        return false;
    }

    struct mmio_region *const completion_queue_mmio =
        vmap_mmio(RANGE_INIT(page_to_phys(completion_queue_pages),
                             PAGE_SIZE << completion_order),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (completion_queue_mmio == NULL) {
        vunmap_mmio(submit_queue_mmio);

        free_pages(submit_queue_pages, submit_order);
        free_pages(completion_queue_pages, completion_order);

        printk(LOGLEVEL_WARN,
               "nvme: failed to vmap submit-queue page for nvme-queue\n");

        return false;
    }

    queue->controller = device;
    queue->submit_queue_pages = submit_queue_pages;
    queue->completion_queue_pages = completion_queue_pages;

    queue->submit_queue_mmio = submit_queue_mmio;
    queue->completion_queue_mmio = completion_queue_mmio;

    queue->free_slot_event = EVENT_INIT();
    queue->lock = SPINLOCK_INIT();
    queue->id = id;

    queue->submit_queue_head = 0;
    queue->completion_queue_head = 0;
    queue->submit_queue_tail = 0;

    queue->submit_doorbell =
        reg_to_ptr(uint32_t, device->regs->doorbell, device->stride * id);

    queue->entry_count = entry_count;
    queue->submit_alloc_order = submit_order;
    queue->completion_alloc_order = completion_order;
    queue->phase = true;

    if (max_transfer_shift != 0) {
        queue->phys_region_pages_count =
            div_round_up(1ull << max_transfer_shift, PAGE_SIZE);

        printk(LOGLEVEL_INFO,
               "nvme: page-count: %" PRIu32 "\n",
               queue->phys_region_pages_count);
    }

    return true;
}

__optimize(3) void nvme_queue_destroy(struct nvme_queue *const queue) {
    vunmap_mmio(queue->submit_queue_mmio);
    vunmap_mmio(queue->completion_queue_mmio);

    free_pages(queue->submit_queue_pages, queue->submit_alloc_order);
    free_pages(queue->completion_queue_pages,
               queue->completion_alloc_order);

    queue->submit_queue_mmio = NULL;
    queue->completion_queue_mmio = NULL;
    queue->id = 0;

    queue->controller = NULL;
    queue->submit_queue_head = 0;
    queue->completion_queue_head = 0;
    queue->submit_queue_tail = 0;

    queue->submit_doorbell = NULL;

    queue->entry_count = 0;
    queue->submit_alloc_order = 0;
    queue->completion_alloc_order = 0;

    queue->phase = false;
}

static inline bool await_completion(struct nvme_queue *const queue) {
    volatile struct nvme_completion_queue_entry *const completion_queue =
        queue->completion_queue_mmio->base;

    do {
        const uint8_t phase = queue->phase;
        uint8_t head = queue->completion_queue_head;

        const uint32_t status = mmio_read(&completion_queue[head].status);
        if ((status & __NVME_COMPL_QUEUE_ENTRY_STATUS_PHASE) != phase) {
            sched_yield();
            continue;
        }

        queue->completion_queue_head++;
        if (queue->completion_queue_head == queue->entry_count) {
            queue->completion_queue_head = 0;
            queue->phase = !queue->phase;
        }

        // Signal completion doorbell
        mmio_write(&queue->submit_doorbell[1], queue->completion_queue_head);
        return (status & __NVME_COMPL_QUEUE_ENTRY_STATUS_CODE) == 0;
    } while (true);
}

bool
nvme_queue_submit_command(struct nvme_queue *const queue,
                          struct nvme_command *const command)
{
    command->common.cid = queue->cmd_identifier;
    if (queue->cmd_identifier != UINT16_MAX) {
        queue->cmd_identifier++;
    } else {
        queue->cmd_identifier = 0;
    }

    const uint8_t tail = queue->submit_queue_tail;
    volatile struct nvme_command *const submit_queue =
        queue->submit_queue_mmio->base;

    submit_queue[tail] = *command;
    queue->submit_queue_tail++;

    if (queue->submit_queue_tail == queue->entry_count) {
        queue->submit_queue_tail = 0;
    }

    mmio_write(queue->submit_doorbell, queue->submit_queue_tail);
    return await_completion(queue);
}