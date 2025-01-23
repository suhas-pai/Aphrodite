/*
 * kernel/include/acpi/device.h
 * © suhas pai
 */

#pragma once

#include <uacpi/types.h>
#include "lib/adt/string_view.h"

#include "dev/device.h"
#include "resources.h"

struct acpi_device {
    struct device device;
    struct string_view name;

    struct os_acpi_device_resources resources;
    uacpi_namespace_node *node;
};
