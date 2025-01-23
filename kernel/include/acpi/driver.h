/*
 * kernel/include/acpi/driver.h
 * © suhas pai
 */

#pragma once

#include <uacpi/utilities.h>
#include "dev/driver.h"

struct acpi_driver {
    const struct string_view *pnp_ids;

    const uint32_t pnp_id_count;
    uint8_t resources_flags;

    struct uacpi_namespace_node *(*get_namespace)(void);
    struct driver driver;
};

void acpi_init_drivers();
