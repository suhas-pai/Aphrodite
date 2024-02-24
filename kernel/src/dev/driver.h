/*
 * kernel/src/dev/driver.h
 * © suhas pai
 */

#pragma once

#include "dtb/driver.h"
#include "pci/driver.h"

struct driver {
    const struct string_view name;

    const struct dtb_driver *dtb;
    const struct pci_driver *pci;
};

extern char drivers_start[];
extern char drivers_end[];

#define driver_foreach(iter) \
    for (struct driver *iter = (struct driver *)(uint64_t)drivers_start; \
         iter < (struct driver *)(uint64_t)drivers_end; \
         iter++) \

#define __driver __attribute__((used, section(".drivers")))
