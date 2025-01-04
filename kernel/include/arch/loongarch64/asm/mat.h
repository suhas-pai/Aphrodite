/*
 * kernel/src/arch/loongarch64/asm/mat.h
 * © suhas pai
 */

#pragma once

enum mem_access_ctrl {
    MEM_ACCESS_CTRL_CACHE_COHERENT = 1,
    MEM_ACCESS_CTRL_WEAKLY_CACHE_COHERENT,
};
