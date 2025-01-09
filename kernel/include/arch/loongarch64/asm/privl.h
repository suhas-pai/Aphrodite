/*
 * kernel/include/arch/loongarch64/asm/privl.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum privl : uint8_t {
    PRIVL_KERNEL,
    PRIVL_USER = 3,
};
