/*
 * kernel/include/arch/riscv64/asm/csr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#define csr_clear(csr, bits) ({ \
    uint64_t __csr_clear_val__ = (bits); \
    asm volatile("csrc " #csr ", %0" \
                 :: "rK" (__csr_clear_val__) \
                 : "memory"); \
})

#define csr_read_clear(csr, bits) ({ \
    uint64_t __csr_read_clear_val__ = (bits); \
    uint64_t __csr_read_clear_val_out__ = 0; \
\
    asm volatile("csrrc %0, " #csr ", %1" \
                 : "=r"(__csr_read_clear_val__) \
                 : "rK" (__csr_read_clear_val__) \
                 : "memory"); \
    __csr_read_clear_val__; \
})

#define csr_set(csr, bits) ({ \
    uint64_t __csr_set_val__ = (bits); \
    asm volatile("csrs " #csr ", %0" \
                 :: "rK" (__csr_set_val__) \
                 : "memory"); \
})

#define csr_read(csr) ({ \
    uint64_t __csr_read_val__ = 0; \
    asm volatile("csrr %0, " #csr \
                 : "=r" (__csr_read_val__) \
                 :: "memory"); \
    __csr_read_val__; \
})

#define csr_write(csr, val) ({ \
    uint64_t __csr_write_val__ = (val); \
    asm volatile("csrw " #csr ", %0" \
                 :: "rK" (__csr_write_val__) \
                 : "memory"); \
    __csr_write_val__; \
})

#define csr_read_and_zero(csr) ({ \
    uint64_t __csr_read_zero_val__ = 0; \
    asm volatile("csrrw %0, " #csr ", zero" \
                 : "=r" (__csr_read_zero_val__) \
                 :: "memory"); \
    __csr_read_zero_val__; \
})
