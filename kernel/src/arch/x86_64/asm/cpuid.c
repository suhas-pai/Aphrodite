/*
 * kernel/src/arch/x86_64/asm/cpuid.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "cpuid.h"

/*
 * Issue a single request to CPUID. Fits 'intel features', for instance note
 * that even if only "eax" and "edx" are of interest, other registers will be
 * modified by the operation, so we need to tell the compiler about it
 */

__debug_optimize(3) void
cpuid(const uint32_t leaf,
      const uint32_t subleaf,
      uint64_t *const a,
      uint64_t *const b,
      uint64_t *const c,
      uint64_t *const d)
{
    asm volatile("cpuid"
               : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
               : "a" (leaf), "c" (subleaf));
}

// Issue a complete request, storing general registers output as a string.
__debug_optimize(3) int cpuid_string(const int code, char string[const 16]) {
    uint32_t *const where = (uint32_t *)(uint64_t)string;
    asm volatile("cpuid"
               : "=a"(where[0]), "=b"(where[1]),
                 "=c"(where[2]), "=d"(where[3])
               : "a"(code));

    return (int)where[0];
}