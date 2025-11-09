/*
 * kernel/src/dev/virtio/queue/split.c
 * © suhas pai
 */

#include <stdatomic.h>
#include <lib/align.h>

#include "dev/virtio/queue/split.h"
#include "dev/virtio/transport.h"

#include "dev/printk.h"
#include "mm/page_alloc.h"

#define VIRTIO_SPLIT_QUEUE_ALIGN PAGE_SIZE

bool
virtio_split_queue_init(struct virtio_device *const device,
                        struct virtio_split_queue *const queue,
                        const uint16_t queue_index)
{
    virtio_device_select_queue(device, queue_index);
    const uint16_t queue_size =
        min(virtio_device_selected_queue_max_size(device),
            VIRTQ_MAX_DESC_COUNT);

    const uint32_t desc_table_size =
        arrptr_size(sizeof(struct virtq_desc), queue_size);

    uint32_t avail_ring_offset = desc_table_size;
    if (device->has_legacy_interface) {
        avail_ring_offset =
            align_up_assert(avail_ring_offset, VIRTIO_SPLIT_QUEUE_ALIGN);
    }

    const uint32_t avail_ring_size =
        sizeof(struct virtq_avail) + arrptr_size(sizeof(le16_t), queue_size);

    const uint32_t used_ring_offset = avail_ring_offset + avail_ring_size;
    const uint32_t used_ring_size =
        sizeof(struct virtq_used) +
        arrptr_size(sizeof(le32_t), VIRTQ_MAX_DESC_COUNT);

    uint16_t page_count = PAGE_COUNT(used_ring_offset + used_ring_size);
    struct page *const page =
        alloc_pages_count(PAGE_STATE_USED,
                          __ALLOC_ZERO,
                          page_count,
                          &page_count);

    if (page == nullptr) {
        printk(LOGLEVEL_WARN,
               "virtio/split-queue: failed to allocate buffer for queue at "
               "index %" PRIu16 "\n",
               queue_index);

        return false;
    }

    void *const page_ptr = page_to_virt(page);

    struct virtq_desc *const desc_table = page_ptr;
    struct virtq_avail *const avail_ring =
        (struct virtq_avail *)(page_ptr + desc_table_size);

    struct virtq_used *const used_ring = (void *)page_ptr + used_ring_offset;
    const uint64_t page_phys = page_to_phys(page);

    virtio_device_set_selected_queue_desc_phys(device, page_phys);
    virtio_device_set_selected_queue_driver_phys(device,
                                                 page_phys + avail_ring_offset);
    virtio_device_set_selected_queue_device_phys(device,
                                                 page_phys + used_ring_offset);

    virtio_device_enable_selected_queue(device);

    // Set the next-indices of each virtio-desc to point to the desc right after
    // Outside the loop, set the next index for the last desc to 0.

    struct virtq_desc *const begin = desc_table;
    const struct virtq_desc *const back = arrptr_back(begin, queue_size - 1);

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
    queue->used_ring = used_ring;

    queue->desc_count = queue_size;
    queue->free_index = 0;
    queue->chain_count = 0;
    queue->index = queue_index;

    return true;
}

void
virtio_split_queue_add(struct virtio_split_queue *const queue,
                       struct virtio_queue_request *const req_list,
                       const uint32_t req_count)
{
    assert_msg(req_count != 0, "virtio/split-queue: add() got req_count=0");
    struct virtio_queue_request *const back = arrptr_back(req_list, req_count);

    const uint16_t head_index = queue->free_index;
    uint16_t free_index = head_index;

    arrptr_foreach(req_list, req_count, req) {
        struct virtq_desc *const desc = &queue->desc_table[free_index];

        desc->phys_addr = (uint64_t)req->data;
        desc->len = req->size;

        if (req != back) {
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