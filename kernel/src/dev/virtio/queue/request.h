/*
 * kernel/src/dev/virtio/queue/request.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum virtio_queue_request_kind {
    VIRTIO_QUEUE_REQUEST_READ,
    VIRTIO_QUEUE_REQUEST_WRITE,
};

struct virtio_queue_request {
    void *data;
    uint32_t size;

    enum virtio_queue_request_kind kind : 1;
};