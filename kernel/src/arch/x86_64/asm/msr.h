/*
 * kernel/arch/x86_64/asm/msr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum ia32_msr {
    IA32_MSR_APIC_BASE = 0x1B,
    IA32_MSR_EFER = 0xC0000080,

    IA32_MSR_FS_BASE = 0xC0000100,
    IA32_MSR_GS_BASE = 0xC0000101,
    IA32_MSR_KERNEL_GS_BASE = 0xC0000102,

    // IA32_MSR_LSTAR is the address SYSCALL jumps to from ring3
    IA32_MSR_LSTAR = 0xC0000082,

    // IA32_MSR_FMASK is a mask of bits to be removed from RFLAGS
    // In SYSCALL, RFLAGS &= ~IA32_MSR_FMASK

    IA32_MSR_FMASK = 0xC0000084,

    /*
     * IA32_MSR_STAR stores the CS (Code Segment) and SS (Stack Segment) info
     * at bits [47:32] and [63:48] respectively.
     */

    IA32_MSR_STAR = 0xC0000081,
    IA32_MSR_TSC_DEADLINE = 0x6E0,

    IA32_MSR_X2APIC_BASE = 0x802,

    IA32_MSR_PAT = 0x277,
    IA32_MSR_MTRR_DEF_TYPE = 0x2FF,
    IA32_MSR_MTRRCAP = 0xFE,
    IA32_MSR_MTRR_PHYSBASE0 = 0x200,
    IA32_MSR_MTRR_PHYSMASK0 = 0x201,
    IA32_MSR_MTRR_PHYSBASE1 = 0x202,

    IA32_MSR_MISC_ENABLE = 0x1a0
};

enum ia32_msr_efer_flags {
    // System Call Extensions
    __IA32_MSR_EFER_BIT_SCE = 1ull << 0,

    // Long Mode Enable
    __IA32_MSR_EFER_BIT_LME = 1ull << 8,

    // Long Mode Active
    __IA32_MSR_EFER_BIT_LMA = 1ull << 10,

    // No Execute Enable
    __IA32_MSR_EFER_BIT_NXE = 1ull << 11,
};

enum ia32_msr_misc_flags {
    __IA32_MSR_MISC_FAST_STRING_ENABLE    = 1ull << 0,
    __IA32_MSR_MISC_POPCODE_COMPAT_ENABLE = 1ull << 2,
    __IA32_MSR_MISC_THERM_MONITOR_ENABLE  = 1ull << 3,
    __IA32_MSR_MISC_SPLIT_LOCK_ENABLE     = 1ull << 4,
    __IA32_MSR_MISC_BUS_LOCK_ENABLE       = 1ull << 8,
    __IA32_MSR_MISC_HW_PREFETCH_DISABLE   = 1ull << 9,
    __IA32_MSR_MISC_GV12_LEGACY_ENABLE    = 1ull << 15,
    __IA32_MSR_MISC_MONITOR_MWAIT_ENABLE  = 1ull << 18,

    __IA32_MSR_MISC_ADJC_SECT_PREFETCH_DISABLE = 1ull << 19
};

enum msr_pat_encoding {
    MSR_PAT_ENCODING_UNCACHEABLE     = 0x00,
    MSR_PAT_ENCODING_WRITE_COMBINING = 0x01,
    MSR_PAT_ENCODING_WRITE_THROUGH   = 0x04,
    MSR_PAT_ENCODING_WRITE_PROTECT   = 0x05,
    MSR_PAT_ENCODING_WRITE_BACK      = 0x06,
    MSR_PAT_ENCODING_UNCACHED        = 0x07,
};

enum msr_pat_indexes {
    // PAT=0, PCD=0, PWT=0
    MSR_PAT_INDEX_PAT0 = 0,

    // PAT=0, PCD=0, PWT=1
    MSR_PAT_INDEX_PAT1 = 8,

    // PAT=0, PCD=1, PWT=0
    MSR_PAT_INDEX_PAT2 = 16,

    // PAT=0, PCD=1, PWT=1
    MSR_PAT_INDEX_PAT3 = 24,

    // PAT=1, PCD=0, PWT=0
    MSR_PAT_INDEX_PAT4 = 32,

    // PAT=1, PCD=0, PWT=1
    MSR_PAT_INDEX_PAT5 = 40,

    // PAT=1, PCD=1, PWT=0
    MSR_PAT_INDEX_PAT6 = 48,

    // PAT=1, PCD=1, PWT=1
    MSR_PAT_INDEX_PAT7 = 56,
};

#define MSR_PAT_ENTRY_MASK 0b111ull

uint64_t read_msr(enum ia32_msr msr);
void write_msr(enum ia32_msr msr, uint64_t value);