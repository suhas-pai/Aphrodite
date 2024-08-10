/*
 * kernel/src/arch/x86_64/asm/msr.c
 * © suhas pai
 */

#include "asm/msr.h"
#include "lib/macros.h"

__debug_optimize(3) uint64_t msr_read(const enum ia32_msr msr) {
    uint32_t eax = 0;
    uint32_t edx = 0;

    asm volatile ("rdmsr"
                  : "=a" (eax), "=d" (edx)
                  : "c" ((uint32_t)msr)
                  : "memory");

    return ((uint64_t)edx << 32 | eax);
}

__debug_optimize(3)
void msr_write(const enum ia32_msr msr, const uint64_t value) {
    uint32_t eax = (uint32_t)value;
    uint32_t edx = value >> 32;

    asm volatile ("wrmsr"
                  :: "a" (eax), "d" (edx), "c" ((uint32_t)msr)
                  : "memory");
}