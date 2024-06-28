/*
 * kernel/src/arch/x86_64/asm/error_code.h
 * © suhas pai
 */

#pragma once

enum page_fault_error_code_flags {
    __PAGE_FAULT_ERROR_CODE_PRESENT = 1 << 0,
    __PAGE_FAULT_ERROR_CODE_WRITE = 1 << 1,
    __PAGE_FAULT_ERROR_CODE_USER = 1 << 2,
    __PAGE_FAULT_ERROR_CODE_RESERVED_WRITE = 1 << 3,
    __PAGE_FAULT_ERROR_CODE_INSTRUCTION_FETCH = 1 << 4,
    __PAGE_FAULT_ERROR_CODE_PROTECTION_KEY = 1 << 5,
    __PAGE_FAULT_ERROR_CODE_SHADOW_STACK = 1 << 6,
    __PAGE_FAULT_ERROR_CODE_SW_GUARD_EXT = 1 << 15,
};
