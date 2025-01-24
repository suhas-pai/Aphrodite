/*
 * kernel/src/dev/device.c
 * © suhas pai
 */

#include "dev/device.h"
#include "dev/driver.h"

void
device_initialize(struct device *const device,
                  struct bus *const bus,
                  struct driver *const driver,
                  const struct string_view init_name)
{
    device->bus = bus;
    device->driver = driver;
    device->init_name = init_name;
    device->lock = SPINLOCK_INIT();
    device->is_bus = true;

    list_init(&device->list);
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_add(&bus->device_list, &device->list);
    });

    if (driver != NULL && driver->bus != NULL && driver->bus != bus) {
        with_spinlock_intr_disabled(&driver->bus->device.lock, {
            list_add(&driver->bus->device_list, &device->list);
        });
    }
}

void
device_init_for_bus(struct device *const device,
                    struct bus *const bus,
                    struct bus *const parent,
                    const struct string_view init_name)
{
    device->bus = bus;
    device->parent = parent;
    device->init_name = init_name;
    device->lock = SPINLOCK_INIT();
    device->is_bus = false;

    list_init(&device->list);
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_add(&bus->device_list, &device->list);
    });

    if (parent != NULL && parent != bus) {
        with_spinlock_intr_disabled(&parent->device.lock, {
            list_add(&parent->device_list, &device->list);
        });
    }
}

__debug_optimize(3) uint64_t device_get_id(struct device *const device) {
    (void)device;
    verify_not_reached();
}

__debug_optimize(3) bool device_probe(struct device *const device) {
    return device->driver->probe(device);
}

__debug_optimize(3) void device_shutdown(struct device *const device) {
    device->driver->shutdown(device);
}

__debug_optimize(3) bool device_suspend(struct device *const device) {
    return device->driver->suspend(device);
}

__debug_optimize(3) bool device_resume(struct device *const device) {
    return device->driver->resume(device);
}

__debug_optimize(3) bool device_remove(struct device *device) {
    return device->driver->remove(device);
}
