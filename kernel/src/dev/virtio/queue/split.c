/*
 * kernel/src/dev/virtio/queue/split.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "dev/printk.h"
#include "lib/align.h"
#include "mm/page_alloc.h"

#include "../transport.h"
#include "split.h"

#define VIRTIO_SPLIT_QUEUE_ALLOC_PAGE_ORDER 0

bool
virtio_split_queue_init(struct virtio_device *const device,
                        struct virtio_split_queue *const queue,
                        const uint16_t queue_index)
{
    virtio_device_select_queue(device, queue_index);
    const uint16_t desc_count =
        min(virtio_device_selected_queue_max_size(device),
            VIRTQ_MAX_DESC_COUNT);

    _Static_assert(
        // Desc Table
        (sizeof(struct virtq_desc) * VIRTQ_MAX_DESC_COUNT)
        // Avail ring
        + (sizeof(struct virtq_avail) + (sizeof(le16_t) * VIRTQ_MAX_DESC_COUNT))
        // Used Ring
        + (sizeof(struct virtq_used)
          + (sizeof(struct virtq_used_elem) * VIRTQ_MAX_DESC_COUNT))
            <= (PAGE_SIZE << VIRTIO_SPLIT_QUEUE_ALLOC_PAGE_ORDER),
        "virtio/split-queue: VIRTIO_SPLIT_QUEUE_ALLOC_PAGE_ORDER needs to be "
        "increased");

    struct page *const page =
        alloc_pages(PAGE_STATE_USED,
                    __ALLOC_ZERO,
                    VIRTIO_SPLIT_QUEUE_ALLOC_PAGE_ORDER);

    if (page == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio/split-queue: failed to allocate buffer for queue at "
               "index %" PRIu16 "\n",
               queue_index);

        return false;
    }

    void *const page_ptr = page_to_virt(page);

    struct virtq_desc *const desc_table = page_ptr;
    struct virtq_avail *const avail_ring =
        (struct virtq_avail *)(desc_table + desc_count);

    const uint32_t avail_ring_size =
        sizeof(struct virtq_avail) + (sizeof(le16_t) * desc_count);
    struct virtq_used *const used_ring =
        (void *)avail_ring + align_up_assert(avail_ring_size, /*boundary=*/4);

    const uint64_t page_phys = page_to_phys(page);
    const uint32_t desc_table_size = sizeof(struct virtq_desc) * desc_count;

    virtio_device_set_selected_queue_desc_phys(device, page_phys);
    virtio_device_set_selected_queue_driver_phys(device,
                                                 page_phys + desc_table_size);
    virtio_device_set_selected_queue_device_phys(device,
                                                 page_phys +
                                                 desc_table_size +
                                                 avail_ring_size);

    virtio_device_enable_selected_queue(device);

    // Set the next-indices of each virtio-desc to point to the desc right after
    // Outside the loop, set the next index for the last desc to 0.

    struct virtq_desc *const begin = desc_table;
    const struct virtq_desc *const back = begin + (desc_count - 1);

    struct virtq_desc *iter = begin;
    uint16_t next_index = 1;

    for (; iter != back; iter++, next_index++) {
        iter->next = next_index;
        iter->flags |= __VIRTQ_DESC_F_NEXT;
    }

    iter->next = 0;

    queue->page = page;
    queue->desc_table = desc_table;
    queue->avail_ring = avail_ring;

    queue->desc_count = desc_count;
    queue->used_ring = used_ring;
    queue->free_index = 0;
    queue->chain_count = 0;
    queue->index = queue_index;

    return true;
}

void
virtio_split_queue_add(struct virtio_split_queue *const queue,
                       struct virtio_queue_request *const req,
                       const uint32_t count)
{
    assert_msg(count != 0, "virtio/split-queue: add() got count=0");

    const uint16_t head_index = queue->free_index;
    uint16_t free_index = head_index;

    for (uint32_t i = 0; i != count; i++) {
        struct virtq_desc *const desc = &queue->desc_table[free_index];

        desc->phys_addr = (uint64_t)req->data;
        desc->len = req->size;

        if (i != count - 1) {
            desc->flags = __VIRTQ_DESC_F_NEXT;
        }

        if (req->kind == VIRTIO_QUEUE_REQUEST_WRITE) {
            desc->flags |= __VIRTQ_DESC_F_WRITE;
        }

        free_index = desc->next;
    }

    const uint16_t avail_index =
        (queue->avail_ring->index + queue->chain_count) % queue->desc_count;

    queue->avail_ring->ring[avail_index] = head_index;
    queue->chain_count += 1;
}

void
virtio_split_queue_commit(struct virtio_device *const device,
                          struct virtio_split_queue *const queue)
{
    // 4. The driver performs a suitable memory barrier to ensure the device
    //    sees the updated descriptor table and available ring before the next
    //    step.
    atomic_thread_fence(memory_order_seq_cst);

    // 5. The available idx is increased by the number of descriptor chain heads
    //    added to the available ring.
    queue->avail_ring->index += queue->chain_count;

    // 6. The driver performs a suitable memory barrier to ensure that it
    //    updates the idx field before checking for notification suppression.
    atomic_thread_fence(memory_order_seq_cst);

    // 7. The driver sends an available buffer notification to the device if
    //    such notifications are not suppressed
    if ((queue->used_ring->flags & __VIRTQ_USED_F_NO_NOTIFY) == 0) {
        virtio_device_notify_queue(device, queue->index);
    }
}