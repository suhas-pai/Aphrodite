/*
 * kernel/include/dev/bus.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string_view.h"
#include "lib/list.h"

#include "device.h"

struct bus;
typedef bool (*bus_probe_t)(struct bus *bus);

struct bus {
    struct device device;

    struct list driver_list;
    struct list device_list;

    bus_probe_t probe;
};

#define BUS_INIT(name, parent, probe_) \
    ((struct bus){ \
        .driver_list = LIST_INIT(name.driver_list), \
        .device_list = LIST_INIT(name.device_list), \
        .probe = (probe_) \
    })

#define bus_foreach_driver(bus, type, field, iter) \
    type *iter = nullptr; \
    list_foreach(iter, &(bus)->driver_list, field) \

#define bus_foreach_device(bus, type, field, iter) \
    type *iter = nullptr; \
    list_foreach(iter, &(bus)->device_list, field) \

void bus_init_root(struct bus *bus);

void
bus_init(struct bus *bus,
         struct bus *parent,
         struct string_view name,
         struct driver *driver,
         bus_probe_t probe);

void
bus_init_no_driver(struct bus *bus,
                   struct bus *parent,
                   struct string_view name,
                   struct bus *parent2,
                   bus_probe_t probe);

struct bus *bus_get_dev_parent(struct bus *bus);
struct bus *bus_get_drv_parent(struct bus *bus);

void bus_subsystem_init();

void bus_register(struct bus *bus);
void bus_unregister(struct bus *bus);

void bus_add_device(struct bus *bus, struct device *device);
void bus_remove_device(struct bus *bus, struct device *device);

void bus_add_driver(struct bus *bus, struct driver *driver);
void bus_remove_driver(struct bus *bus, struct driver *driver);

bool bus_probe(struct bus *bus);
