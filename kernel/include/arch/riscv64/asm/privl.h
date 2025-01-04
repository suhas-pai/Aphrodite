/*
 * kernel/src/arch/riscv64/asm/privl.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum riscv64_privl : uint8_t {
    RISCV64_PRIVL_MACHINE,
    RISCV64_PRIVL_SUPERVISOR,
};
