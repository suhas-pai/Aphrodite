/*
 * kernel/dev/virtio/drivers/block.h
 * © suhas pai
 */

#pragma once
#include "dev/virtio/device.h"

struct virtio_block_host_device {

};

struct virtio_device *
virtio_block_driver_init(struct virtio_device *device, uint64_t features);