/*
 * kernel/include/arch/loongarch64/asm/mat.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum mem_access_ctrl : uint8_t {
    MEM_ACCESS_CTRL_CACHE_COHERENT = 1,
    MEM_ACCESS_CTRL_WEAKLY_CACHE_COHERENT,
};
