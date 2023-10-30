/*
 * kernel/src/arch/riscv64/asm/status.h
 * © suhas pai
 */

#pragma once

enum mstatus_shifts {
    MSTATUS_VM_SHIFT = 1
};

enum mstatus_flags {
    __MSTATUS_INTERRUPT_ENABLE = 1ull << 0,
    __MSTATUS_VIRTUALIZATION_MGMT = 0b11111 << 1,
};