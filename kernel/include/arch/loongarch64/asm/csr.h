/*
 * kernel/include/arch/loongarch64/asm/csr.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum csr : uint8_t {
    CSR_crmd,
    CSR_ecfg = 4,
    CSR_estat,
    CSR_era,
    CSR_badv,
    CSR_badi,
    CSR_eentry = 0xC,
    CSR_asid = 0x18,
    CSR_pgdl,
    CSR_pgdh,
    CSR_pgd,
    CSR_cpuid = 0x20,
    CSR_tid = 0x40,
    CSR_tcfg,
    CSR_tval,
};

#define csr_read(csr) ({ \
    uint64_t __csr_read_val__ = 0; \
    asm volatile("csrrd %0, %1" \
                 : "=r" (__csr_read_val__) \
                 : "i"(VAR_CONCAT(CSR_, csr))); \
    __csr_read_val__; \
})

#define csr_write(csr, val) ({ \
    uint64_t __csr_write_val__ = (val); \
    asm volatile("csrwr %0, %1" \
                 :: "r" (__csr_write_val__), "i"(VAR_CONCAT(CSR_, csr)) \
                 : "memory"); \
    __csr_write_val__; \
})
