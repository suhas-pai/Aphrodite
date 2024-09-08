
/*
 * kernel/src/dev/nvme/queue.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "mm/page_alloc.h"
#include "mm/phalloc.h"

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

    struct page *const submit_queue_page =
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, submit_order);

    if (submit_queue_page == NULL) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to allocate page for nvme submit-queue\n");
        return false;
    }

    const uint64_t submit_queue_phys = page_to_phys(submit_queue_page);
    const uint32_t completion_alloc_size =
        sizeof(struct nvme_completion_queue_entry) * entry_count;

    uint8_t completion_order = 0;
    while ((PAGE_SIZE << completion_order) < completion_alloc_size) {
        completion_order++;
    }

    struct page *const completion_queue_page =
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, completion_order);

    if (completion_queue_page == NULL) {
        free_pages(submit_queue_page, submit_order);
        printk(LOGLEVEL_WARN,
               "nvme: failed to allocate page for nvme completion-queue\n");

        return false;
    }

    const uint64_t completion_queue_phys = page_to_phys(completion_queue_page);
    struct mmio_region *const submit_queue_mmio =
        vmap_mmio(RANGE_INIT(submit_queue_phys,
                             align_up_assert(submit_alloc_size, PAGE_SIZE)),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (submit_queue_mmio == NULL) {
        free_pages(submit_queue_page, submit_order);
        free_pages(completion_queue_page, completion_order);

        printk(LOGLEVEL_WARN,
               "nvme: failed to vmap submit-queue page for nvme-queue\n");
        return false;
    }

    struct mmio_region *const completion_queue_mmio =
        vmap_mmio(RANGE_INIT(completion_queue_phys,
                             align_up_assert(completion_alloc_size, PAGE_SIZE)),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (completion_queue_mmio == NULL) {
        vunmap_mmio(submit_queue_mmio);

        free_pages(submit_queue_page, submit_order);
        free_pages(completion_queue_page, completion_order);

        printk(LOGLEVEL_WARN,
               "nvme: failed to vmap submit-queue page for nvme-queue\n");

        return false;
    }

    queue->controller = device;
    queue->submit_queue_phys = submit_queue_phys;
    queue->completion_queue_phys = completion_queue_phys;

    queue->submit_queue_mmio = submit_queue_mmio;
    queue->completion_queue_mmio = completion_queue_mmio;

    queue->lock = SPINLOCK_INIT();
    queue->event = EVENT_INIT();
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
    queue->phase = true;

    queue->submit_alloc_order = submit_order;
    queue->completion_alloc_order = completion_order;

    if (max_transfer_shift != 0) {
        queue->phys_region_pages_count =
            div_round_up(1ull << max_transfer_shift, PAGE_SIZE);

        const uint64_t phys_region_page_list_size =
            queue->phys_region_pages_count
          * queue->entry_count
          * sizeof(uint64_t);

        const uint64_t prp_phys = phalloc(phys_region_page_list_size);
        if (prp_phys == INVALID_PHYS) {
            vunmap_mmio(submit_queue_mmio);

            phalloc_free(submit_queue_phys);
            phalloc_free(completion_queue_phys);

            printk(LOGLEVEL_WARN,
                   "nvme: failed to alloc list of physical region pages\n");

            return false;
        }

        queue->phys_region_page_list = phys_to_virt(prp_phys);
    } else {
        queue->phys_region_page_list = NULL;
    }

    return true;
}

__debug_optimize(3)
uint16_t nvme_queue_get_cmdid(struct nvme_queue *const queue) {
    uint16_t result = 0;
    SPIN_WITH_IRQ_ACQUIRED(&queue->lock, {
        result = queue->cmd_identifier;
        queue->cmd_identifier++;

        if (queue->cmd_identifier >= queue->entry_count) {
            queue->cmd_identifier = 0;
        }
    });

    return result;
}

__debug_optimize(3) void nvme_queue_destroy(struct nvme_queue *const queue) {
    if (queue->phys_region_page_list != NULL) {
        phalloc_free(virt_to_phys(queue->phys_region_page_list));

        queue->phys_region_page_list = NULL;
        queue->phys_region_pages_count = 0;
    }

    vunmap_mmio(queue->submit_queue_mmio);
    vunmap_mmio(queue->completion_queue_mmio);

    free_pages(phys_to_page(queue->submit_queue_phys),
               queue->submit_alloc_order);
    free_pages(phys_to_page(queue->completion_queue_phys),
               queue->completion_alloc_order);

    queue->submit_queue_mmio = NULL;
    queue->completion_queue_mmio = NULL;
    queue->id = 0;

    queue->controller = NULL;
    queue->submit_queue_head = 0;
    queue->completion_queue_head = 0;
    queue->submit_queue_tail = 0;

    queue->doorbells = NULL;
    queue->entry_count = 0;
    queue->phase = false;
}

bool
nvme_queue_submit_command(struct nvme_queue *const queue,
                          const struct nvme_command *const command)
{
    SPIN_WITH_IRQ_ACQUIRED(&queue->lock, {
        const uint8_t tail = queue->submit_queue_tail;
        volatile struct nvme_command *const submit_queue =
            queue->submit_queue_mmio->base;

        submit_queue[tail] = *command;
        queue->submit_queue_tail++;

        if (queue->submit_queue_tail == queue->entry_count) {
            queue->submit_queue_tail = 0;
        }

        mmio_write(&queue->doorbells->submit, queue->submit_queue_tail);
    });

    struct event *const event = &queue->event;
    events_await(&event,
                 /*events_count=*/1,
                 /*block=*/true,
                 /*drop_after_recv=*/true);

    return true;
}