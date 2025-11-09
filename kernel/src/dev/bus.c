/*
 * kernel/src/dev/bus.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/init.h"

static struct list g_bus_list = LIST_INIT(g_bus_list);
static struct spinlock g_lock = SPINLOCK_INIT();

void
bus_init(struct bus *const bus,
         struct bus *const parent,
         const struct string_view name,
         struct bus *const parent2,
         const bus_probe_t probe)
{
    device_init_for_bus(&bus->device, parent, parent2, name);

    list_init(&bus->driver_list);
    list_init(&bus->device_list);

    bus->probe = probe;
}

__debug_optimize(3) void bus_init_root(struct bus *const bus) {
    bus->device.bus = dev_root_bus();
}

__debug_optimize(3) struct bus *bus_get_dev_parent(struct bus *const bus) {
    return bus->device.bus;
}

__debug_optimize(3) struct bus *bus_get_driver_parent(struct bus *const bus) {
    assert(bus->device.is_bus);
    return bus->device.driver->bus;
}

void bus_subsystem_init() {
    struct bus *bus = nullptr;
    list_foreach(&g_bus_list, device.list, bus) {
        bus->probe(bus);
    }
}

void bus_register(struct bus *const bus) {
    with_spinlock_intr_disabled(&g_lock, {
        list_add(&g_bus_list, &bus->device.list);
    });
}

void bus_unregister(struct bus *const bus) {
    with_spinlock_intr_disabled(&g_lock, {
        list_remove(&bus->device.list);
    });
}

void bus_add_device(struct bus *const bus, struct device *const device) {
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_radd(&bus->device_list, &device->list);
    });
}

void bus_remove_device(struct bus *const bus, struct device *const device) {
    assert(device->bus == bus);
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_remove(&device->list);
    });
}

void bus_add_driver(struct bus *const bus, struct driver *const driver) {
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_radd(&bus->driver_list, &driver->list);
    });
}
void bus_remove_driver(struct bus *const bus, struct driver *const driver) {
    assert(driver->bus == bus);
    with_spinlock_intr_disabled(&bus->device.lock, {
        list_remove(&driver->list);
    });
}

bool bus_probe(struct bus *const bus) {
    if (bus->probe != nullptr) {
        return bus->probe(bus);
    }

    return true;
}
