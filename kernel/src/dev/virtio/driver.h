/*
 * kernel/dev/virtio/driver.h
 * © suhas pai
 */

#pragma once
#include "device.h"

typedef struct virtio_device *
(*virtio_driver_init_t)(struct virtio_device *device, uint64_t features);
