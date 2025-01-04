/*
 * kernel/src/arch/loongarch64/asm/privl.h
 * © suhas pai
 */

#pragma once

enum privl {
    PRIVL_KERNEL,
    PRIVL_USER = 3,
};
