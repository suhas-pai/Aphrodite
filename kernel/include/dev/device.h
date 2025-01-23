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
        // Nodes that are busses don't have individual drivers, and instead have
        // a second bus that they're a child of.

        struct driver *driver;
        struct bus *parent;
    };

    struct string_view init_name;
    struct spinlock lock;

    struct list list;
    struct list child_list;

    bool has_driver : 1;
};

void
device_initialize(struct device *device,
                  struct bus *bus,
                  struct driver *driver,
                  struct string_view init_name);

void
device_init_no_driver(struct device *device,
                      struct bus *bus,
                      struct bus *parent,
                      struct string_view init_name);

uint64_t device_get_id(struct device *device);

bool device_probe(struct device *device);
void device_shutdown(struct device *device);
bool device_suspend(struct device *device);
bool device_resume(struct device *device);
bool device_remove(struct device *device);
