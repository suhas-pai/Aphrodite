/*
 * kernel/dev/virtio/queue.h
 * © suhas pai
 */

#pragma once
#include "dev/virtio/device.h"

struct virtio_split_queue {
    struct page *desc_pages;
    struct page *avail_page;
    struct page *used_pages;

    uint8_t desc_pages_order;
    uint8_t used_pages_order;
};

bool
virtio_split_queue_init(struct virtio_device *device,
                        struct virtio_split_queue *queue,
                        uint16_t queue_index);