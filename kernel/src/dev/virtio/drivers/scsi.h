/*
 * kernel/dev/virtio/drivers/scsi.h
 * © suhas pai
 */

#pragma once
#include "dev/virtio/device.h"

struct virtio_scsi_host_device {

};

struct virtio_device *
virtio_scsi_driver_init(struct virtio_device *device, uint64_t features);