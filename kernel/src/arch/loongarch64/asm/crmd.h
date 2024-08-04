/*
 * kernel/src/arch/loongarch64/asm/crmd.h
 * © suhas pai
 */

#pragma once

enum crmd {
    __CRMD_PRIVL = 0b11 << 0,
    __CRMD_INTR_ENABLE = 1 << 2,
    __CRMD_DIRECT_ACCESS_MEM = 1 << 3,
    __CRMD_PAGING_ENABLE = 1 << 4,

    __CRMD_DIRECT_ACCESS_FETCH_MAT = 0b11 << 5,
    __CRMD_DIRECT_ACCESS_LOAD_STORE_MAT = 0b11 << 7,

    __CRMD_INSTR_DATA_WATCHPOINT_ENABLE = 0b11 << 9,
};
