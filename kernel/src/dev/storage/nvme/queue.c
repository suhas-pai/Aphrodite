
/*
 * kernel/src/dev/nvme/queue.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/page_alloc.h"

#include "sched/scheduler.h"
#include "sys/mmio.h"

#include "controller.h"

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
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, submit_order);

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
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, completion_order);

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

    queue->lock = SPINLOCK_INIT();
    queue->id = id;

    queue->submit_queue_head = 0;
    queue->completion_queue_head = 0;
    queue->submit_queue_tail = 0;

    queue->doorbells =
        reg_to_ptr(struct nvme_queue_doorbells,
                   device->regs->doorbell,
                   device->stride * id);

    queue->entry_count = entry_count;
    queue->cmd_identifier = 0;
    queue->submit_alloc_order = submit_order;
    queue->completion_alloc_order = completion_order;
    queue->phase = true;

    if (max_transfer_shift != 0) {
        queue->phys_region_pages_count =
            div_round_up(1ull << max_transfer_shift, PAGE_SIZE);

        const uint64_t phys_region_page_list_size =
            queue->phys_region_pages_count *
            queue->entry_count *
            sizeof(uint64_t);

        uint8_t order = 0;
        while ((PAGE_SIZE << order) < phys_region_page_list_size) {
            order++;
        }

        struct page *const page =
            alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, order);

        if (page == NULL) {
            vunmap_mmio(submit_queue_mmio);

            free_pages(submit_queue_pages, submit_order);
            free_pages(completion_queue_pages, completion_order);

            printk(LOGLEVEL_WARN,
                   "nvme: failed to alloc list of physical region pages\n");

            return false;
        }

        queue->phys_region_page_list = page_to_virt(page);
    } else {
        queue->phys_region_page_list = NULL;
    }

    return true;
}

__optimize(3) uint16_t nvme_queue_get_cmdid(struct nvme_queue *const queue) {
    const int flag = spin_acquire_with_irq(&queue->lock);
    const uint16_t result = queue->cmd_identifier;

    queue->cmd_identifier++;
    if (queue->cmd_identifier >= queue->entry_count) {
        queue->cmd_identifier = 0;
    }

    spin_release_with_irq(&queue->lock, flag);
    return result;
}

__optimize(3) void nvme_queue_destroy(struct nvme_queue *const queue) {
    if (queue->phys_region_page_list != NULL) {
        const uint64_t phys_region_page_list_size =
            queue->phys_region_pages_count *
            queue->entry_count *
            sizeof(uint64_t);

        uint8_t order = 0;
        while ((PAGE_SIZE << order) < phys_region_page_list_size) {
            order++;
        }

        free_pages(virt_to_page(queue->phys_region_page_list), order);

        queue->phys_region_page_list = NULL;
        queue->phys_region_pages_count = 0;
    }

    vunmap_mmio(queue->submit_queue_mmio);
    vunmap_mmio(queue->completion_queue_mmio);

    free_pages(queue->submit_queue_pages, queue->submit_alloc_order);
    free_pages(queue->completion_queue_pages, queue->completion_alloc_order);

    queue->submit_queue_mmio = NULL;
    queue->completion_queue_mmio = NULL;
    queue->id = 0;

    queue->controller = NULL;
    queue->submit_queue_head = 0;
    queue->completion_queue_head = 0;
    queue->submit_queue_tail = 0;

    queue->doorbells = NULL;

    queue->entry_count = 0;
    queue->submit_alloc_order = 0;
    queue->completion_alloc_order = 0;

    queue->phase = false;
}

__optimize(3)
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

        mmio_write(&queue->doorbells->complete, queue->completion_queue_head);
        return (status & __NVME_COMPL_QUEUE_ENTRY_STATUS_CODE) == 0;
    } while (true);
}

bool
nvme_queue_submit_command(struct nvme_queue *const queue,
                          const struct nvme_command *const command)
{
    const int flag = spin_acquire_with_irq(&queue->lock);

    const uint8_t tail = queue->submit_queue_tail;
    volatile struct nvme_command *const submit_queue =
        queue->submit_queue_mmio->base;

    submit_queue[tail] = *command;
    queue->submit_queue_tail++;

    if (queue->submit_queue_tail == queue->entry_count) {
        queue->submit_queue_tail = 0;
    }

    mmio_write(&queue->doorbells->submit, queue->submit_queue_tail);
    spin_release_with_irq(&queue->lock, flag);

    return await_completion(queue);
}