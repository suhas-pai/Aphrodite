/*
 * kernel/src/dev/virtio/queue/packed.c
 * © suhas pai
 */

#include <lib/align.h>

#include "dev/virtio/queue/packed.h"
#include "dev/virtio/transport.h"

#include "dev/printk.h"
#include "mm/physalloc.h"

#define VIRTIO_PACKED_QUEUE_MAX_DESC_COUNT (1ull << 15)
#define VIRTIO_PACKED_QUEUE_ALIGN (1ull << 12)

struct pvirtq_info {
    volatile struct pvirtq_desc_event event_table;
    volatile struct pvirtq_desc_event avail_table;
};

__unused static inline
struct pvirtq_info *pvirtq_get_info(struct virtio_packed_queue *const queue) {
    const uint64_t count = queue->desc_count;
    return (struct pvirtq_info *)arrptr_end(queue->desc_list, count);
}

bool
virtio_packed_queue_init(struct virtio_device *const device,
                         struct virtio_packed_queue *const queue,
                         const uint16_t queue_index)
{
    virtio_device_select_queue(device, queue_index);
    const uint16_t queue_size =
        min(virtio_device_selected_queue_max_size(device),
            VIRTQ_MAX_DESC_COUNT);

    const uint32_t total_size =
        sizeof(struct pvirtq_desc_event) * queue_size +
        sizeof(struct pvirtq_desc_event) + // Avail event table
        sizeof(struct pvirtq_desc_event); // Used event table

    const uint64_t phys = phys_alloc(total_size);
    if (phys == INVALID_PHYS) {
        printk(LOGLEVEL_WARN,
               "virtio/packed-queue: failed to allocate buffer for queue at "
               "index %" PRIu16 "\n",
               queue_index);

        return false;
    }

    queue->desc_list = phys_to_virt(phys);
    queue->desc_count = queue_size;

    return true;
}

void
virtio_packed_queue_add(struct virtio_packed_queue *const queue,
                        struct virtio_queue_request *const req,
                        const uint32_t count)
{
    (void)queue;
    (void)req;
    (void)count;
}
