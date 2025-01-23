/*
 * kernel/include/dev/driver.h
 * © suhas pai
 */

#pragma once

#include <lib/adt/string_view.h>
#include "dev/bus.h"

typedef bool (*driver_probe_t)(struct device *device);
typedef bool (*driver_remove_t)(struct device *device);
typedef void (*driver_shutdown_t)(struct device *device);
typedef bool (*driver_suspend_t)(struct device *device);
typedef bool (*driver_resume_t)(struct device *device);

struct device;
struct driver {
    struct bus *bus;
    struct string_view name;

    struct list list;

    driver_probe_t probe;
    driver_remove_t remove;
    driver_shutdown_t shutdown;
    driver_suspend_t suspend;
    driver_resume_t resume;
};

void
driver_initialize(struct driver *driver,
                  struct bus *bus,
                  struct string_view name,
                  driver_probe_t probe,
                  driver_remove_t remove,
                  driver_shutdown_t shutdown,
                  driver_suspend_t suspend,
                  driver_resume_t resume);
