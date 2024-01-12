/*
 * kernel/src/arch/riscv64/asm/satp.h
 * © suhas pai
 */

#pragma once

#include "asm/csr.h"
#include "lib/macros.h"

enum satp_mode {
    SATP_MODE_NO_PAGING,

    SATP_MODE_39_BIT_PAGING = 8,
    SATP_MODE_48_BIT_PAGING,
    SATP_MODE_57_BIT_PAGING,
    SATP_MODE_64_BIT_PAGING,
};

enum satp_shifts {
    SATP_PHYS_MODE_SHIFT = 60
};

enum satp_flags {
    __SATP_PHYS_ROOT_NUM = mask_for_n_bits(44),
    __SATP_PHYS_ASID = 0xFFull << 44,
    __SATP_PHYS_MODE = 0xFull << SATP_PHYS_MODE_SHIFT,
};

__optimize(3) static inline uint64_t read_satp() {
    return csr_read(satp);
}

__optimize(3) static inline void write_satp(const uint64_t value) {
    csr_write(satp, value);
}