/*
 * kernel/src/dev/driver.c
 * © suhas pai
 */

#include "dev/driver.h"

void
driver_initialize(struct driver *const driver,
                  struct bus *const bus,
                  const struct string_view name,
                  const driver_probe_t probe,
                  const driver_remove_t remove,
                  const driver_shutdown_t shutdown,
                  const driver_suspend_t suspend,
                  const driver_resume_t resume)
{
    driver->bus = bus;
    driver->name = name;
    driver->probe = probe;
    driver->remove = remove;
    driver->shutdown = shutdown;
    driver->suspend = suspend;
    driver->resume = resume;

    list_init(&driver->list);
    bus_add_driver(bus, driver);
}
