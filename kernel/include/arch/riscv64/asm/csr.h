/*
 * kernel/src/arch/riscv64/asm/csr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#define csr_clear(csr, bits) ({ \
    uint64_t __csrclear_val__ = (bits); \
    asm volatile("csrc " #csr ", %0" \
                 :: "rK" (__csrclear_val__) \
                 : "memory"); \
})

#define csr_read_clear(csr, bits) ({ \
    uint64_t __csrreadclear_val__ = (bits); \
    uint64_t __csrreadclear_val_out__ = 0; \
\
    asm volatile("csrrc %0, " #csr ", %1" \
                 : "=r"(__csrreadclear_val__) \
                 : "rK" (__csrreadclear_val__) \
                 : "memory"); \
    __csrreadclear_val__; \
})

#define csr_set(csr, bits) ({ \
    uint64_t __csrset_val__ = (bits); \
    asm volatile("csrs " #csr ", %0" \
                 :: "rK" (__csrset_val__) \
                 : "memory"); \
})

#define csr_read(csr) ({ \
    uint64_t __csrread_val__ = 0; \
    asm volatile("csrr %0, " #csr \
                 : "=r" (__csrread_val__) \
                 :: "memory"); \
    __csrread_val__; \
})

#define csr_write(csr, val) ({ \
    uint64_t __csrwrite_val__ = (val); \
    asm volatile("csrw " #csr ", %0" \
                 :: "rK" (__csrwrite_val__) \
                 : "memory"); \
    __csrwrite_val__; \
})

#define csr_read_and_zero(csr) ({ \
    uint64_t __csrreadzero_val__ = 0; \
    asm volatile("csrrw %0, " #csr ", zero" \
                 : "=r" (__csrreadzero_val__) \
                 :: "memory"); \
    __csrreadzero_val__; \
})
