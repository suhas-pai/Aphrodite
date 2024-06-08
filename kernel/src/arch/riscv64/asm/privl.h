/*
 * kernel/src/arch/riscv64/asm/privl.h
 * © suhas pai
 */

#pragma once

enum riscv64_privl {
    RISCV64_PRIVL_MACHINE,
    RISCV64_PRIVL_SUPERVISOR,
};
