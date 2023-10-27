/*
 * kernel/dev/dtb/driver.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct dtb_driver {
    bool (*init)(const void *dtb, int nodeoff);

    const char *const *compat_list;
    const uint32_t compat_count;
};