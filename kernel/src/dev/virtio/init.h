/*
 * kernel/dev/virtio/init.h
 * © suhas pai
 */

#pragma once
#include "dev/virtio/device.h"

bool
virtio_device_init_queues(struct virtio_device *device, uint16_t queue_count);