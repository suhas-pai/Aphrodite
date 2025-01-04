/*
 * kernel/src/dev/virtio/init.h
 * © suhas pai
 */

#pragma once
#include "device.h"

bool
virtio_device_init_queues(struct virtio_device *device, uint16_t queue_count);

struct virtio_device *virtio_device_init(struct virtio_device *device);
