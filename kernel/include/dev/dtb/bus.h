/*
 * kernel/include/dev/dtb/bus.h
 * © suhas pai
 */

#pragma once

#include <lib/adt/array.h>
#include "dev/bus.h"

struct dtb_bus {
    struct bus bus;
    struct array device_list;
};

struct dtb_bus *dtb_bus();
