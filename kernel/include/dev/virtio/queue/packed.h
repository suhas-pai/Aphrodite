/*
 * kernel/include/dev/virtio/queue/packed.h
 * © suhas pai
 */

#pragma once

#include "dev/virtio/queue/request.h"
#include "dev/virtio/device.h"

struct virtio_packed_queue {
    struct pvirtq_desc *desc_list;
    uint32_t desc_count;
};

bool
virtio_packed_queue_init(struct virtio_device *device,
                         struct virtio_packed_queue *queue,
                         uint16_t queue_index);

void
virtio_packed_queue_add(struct virtio_packed_queue *queue,
                        struct virtio_queue_request *req,
                        uint32_t count);

void
virtio_packed_queue_commit(struct virtio_device *device,
                           struct virtio_packed_queue *queue);
