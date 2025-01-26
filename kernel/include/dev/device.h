/*
 * kernel/include/dev/device.h
 * © suhas pai
 */

#pragma once

#include <lib/adt/string_view.h>
#include <lib/list.h>

#include "cpu/spinlock.h"

struct bus;
struct driver;
struct device;

struct device {
    struct bus *bus;

    union {
        // Devices that are busses don't have drivers. Instead, a second bus is
        // used as a parent of this device, but for a different tree.

        struct driver *driver;
        struct bus *parent;
    };

    struct string_view init_name;
    struct spinlock lock;

    struct list list;
    bool is_bus : 1;
};

void
device_initialize(struct device *device,
                  struct bus *bus,
                  struct driver *driver,
                  struct string_view init_name);

void
device_init_for_bus(struct device *device,
                    struct bus *bus,
                    struct bus *parent,
                    struct string_view init_name);

uint64_t device_get_id(struct device *device);

bool device_probe(struct device *device);
void device_shutdown(struct device *device);
bool device_suspend(struct device *device);
bool device_resume(struct device *device);
bool device_remove(struct device *device);
