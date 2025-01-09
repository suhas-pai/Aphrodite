/*
 * kernel/include/arch/riscv64/asm/satp.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

enum satp_mode : uint8_t {
    SATP_MODE_NO_PAGING,

    SATP_MODE_39_BIT_PAGING = 8,
    SATP_MODE_48_BIT_PAGING,
    SATP_MODE_57_BIT_PAGING,
    SATP_MODE_64_BIT_PAGING,
};

enum satp_shifts : uint8_t {
    SATP_PHYS_ASID_SHIFT = 44,
    SATP_PHYS_MODE_SHIFT = 60
};

enum satp_flags : uint64_t {
    __SATP_PHYS_ROOT_NUM = mask_for_n_bits(44),
    __SATP_PHYS_ASID = 0xFFull << SATP_PHYS_ASID_SHIFT,
    __SATP_PHYS_MODE = 0xFull << SATP_PHYS_MODE_SHIFT,
};
