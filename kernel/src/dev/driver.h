/*
 * kernel/dev/driver.h
 * © suhas pai
 */

#pragma once

#include "dtb/driver.h"
#include "pci/driver.h"

#include "virtio/driver.h"

struct driver {
    struct dtb_driver *dtb;
    struct pci_driver *pci;
    struct virtio_driver *virtio;
};

extern struct driver drivers_start;
extern struct driver drivers_end;

#define driver_foreach(iter) \
    for (struct driver *iter = &drivers_start; \
         iter < &drivers_end;                  \
         iter++)                               \

#define __driver __attribute__((used, section(".drivers")))
