/*
 * kernel/src/dev/device.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum device_kind {
    DEVICE_KIND_PCI_ENTITY,
};

struct device {
    enum device_kind kind;
};

#define DEVICE_INIT(kind_) \
    ((struct device){ \
        .kind = (kind_) \
    })

uint64_t device_get_id(struct device *device);
