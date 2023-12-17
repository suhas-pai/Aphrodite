/*
 * kernel/src/dev/virtio/queue/split.h
 * © suhas pai
 */

#pragma once

#include "../device.h"
#include "request.h"

struct virtio_split_queue {
    struct page *page;

    struct virtq_desc *desc_table;
    struct virtq_avail *avail_ring;
    struct virtq_used *used_ring;

    volatile uint16_t *notify_ptr;

    uint16_t desc_count;
    uint16_t free_index;
    uint16_t chain_count;
    uint16_t index;
};

bool
virtio_split_queue_init(struct virtio_device *device,
                        struct virtio_split_queue *queue,
                        uint16_t queue_index);

void
virtio_split_queue_add(struct virtio_split_queue *queue,
                       struct virtio_queue_request *req,
                       uint32_t count);

void
virtio_split_queue_transmit(struct virtio_device *device,
                            struct virtio_split_queue *queue);