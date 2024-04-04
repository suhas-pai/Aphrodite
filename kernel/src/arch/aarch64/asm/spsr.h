/*
 * kernel/src/arch/aarch64/asm/spsr.h
 * © suhas pai
 */

#pragma once

enum spsr_mode {
    SPSR_MODE_EL0t,
    SPSR_MODE_EL1t = 0b100,
    SPSR_MODE_EL1h
};

enum spsr_shifts {
    SPSR_EXEC_IS_32B_SHIFT = 4,
    SPSR_SERROR_SHIFT = 8,
    SPSR_DEBUG_SHIFT = 9,
    SPSR_BTI_SHIFT = 10,
};

enum spsr_flags {
    __SPSR_EXCP_LVL_AND_SP = 0b111ull,
    __SPSR_EXEC_IS_32B = 1ull << SPSR_EXEC_IS_32B_SHIFT,
    __SPSR_FIQ_INT = 1ull << 7,
    __SPSR_INTERRUPT = 1ull << 7,
    __SPSR_SERROR = 1ull << SPSR_SERROR_SHIFT,
    __SPSR_DEBUG = 1ull << SPSR_DEBUG_SHIFT,
    __SPSR_BTI = 0b11ull << SPSR_BTI_SHIFT,
    __SPSR_SPECULATIVE_STORE_BYPASS = 1ull << 12,
    __SPSR_ILLEGAL_EXEC = 1ull << 20,
    __SPSR_SOFTWARE_STEP = 1ull << 21,
    __SPSR_PRIVL_ACCESS_NEVER = 1ull << 22,
    __SPSR_USER_ACCESS_OVERRIDE = 1ull << 23,
    __SPSR_DATA_INDEPENDENT_TIMING = 1ull << 24,
    __SPSR_TAG_CHECK_OVERRIDE = 1ull << 25,
    __SPSR_OVERFLOW_CHECK = 1ull << 28,
    __SPSR_CARRY_CONDITION = 1ull << 29,
    __SPSR_ZERO_CONDITION = 1ull << 30,
    __SPSR_NEGATIVE_CONDITION = 1ull << 31,
};
