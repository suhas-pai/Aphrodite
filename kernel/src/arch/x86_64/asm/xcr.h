/*
 * kernel/src/arch/x86_64/asm/xcr.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

enum xcr0_flags {
    // This bit 0 must be 1. An attempt to write 0 to this bit causes a #GP
    // exception.
    __XCR0_BIT_X87 = 1ull << 0,

    // If 1, the XSAVE feature set can be used to manage MXCSR and the XMM
    // registers (XMM0- XMM15 in 64-bit mode; otherwise XMM0-XMM7)
    __XCR0_BIT_SSE = 1ull << 1,

    /*
     * If 1, Intel AVX instructions can be executed and the XSAVE feature set
     * can be used to manage the upper halves of the YMM registers (YMM0-YMM15
     * in 64-bit mode; otherwise YMM0-YMM7).
     */
    __XCR0_BIT_AVX = 1ull << 2,

    // XCR0.BNDREG (bit 3): If 1, Intel MPX instructions can be executed and the
    // XSAVE feature set can be used to manage the bounds registers BND0–BND3.
    __XCR0_BIT_BNDREG = 1ull << 3,

    // If 1, Intel MPX instructions can be executed and the XSAVE feature set
    // can be used to manage the BNDCFGU and BNDSTATUS registers
    __XCR0_BIT_BNDCSR = 1ull << 4,

    // If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
    // set can be used to manage the opmask registers k0–k7
    __XCR0_BIT_OPMASK = 1ull << 5,

    /*
     * If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
     * set can be used to manage the upper halves of the lower ZMM registers
     * (ZMM0-ZMM15 in 64-bit mode; otherwise ZMM0- ZMM7).
     */
    __XCR0_BIT_ZMM_HI256 = 1ull << 6,

    /*
     * If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
     * set can be used to manage the upper ZMM registers (ZMM16-ZMM31, only in
     * 64-bit mode).
     */
    __XCR0_BIT_HI16_ZMM = 1ull << 7,

    // Bit 8 is reserved

    // If 1, the XSAVE feature set can be used to manage the PKRU register
    __XCR0_BIT_PKRU = 1ull << 9,

    // Bits 10 - 16 are reserved

    // If 1, and if XCR0.TILEDATA is also 1, Intel AMX instructions can be
    // executed and the XSAVE feature set can be used to manage TILECFG
    __XCR0_BIT_TILECFG = 1ull << 17,

    // If 1, and if XCR0.TILECFG is also 1, Intel AMX instructions can be
    // executed and the XSAVE feature set can be used to manage TILEDATA
    __XCR0_BIT_TILEDATA = 1ull << 18,
};

enum xcr {
    XCR_XSTATE_FEATURES_ENABLED,
    XCR_XSTATE_FEATURES_IN_USE
};

__optimize(3) static inline uint64_t read_xcr(const enum xcr xcr) {
    uint32_t eax = 0;
    uint32_t edx = 0;

    asm volatile ("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
    return (uint64_t)edx << 32 | eax;
}

__optimize(3)
static inline void write_xcr(const enum xcr xcr, const uint64_t val) {
    asm volatile ("xsetbv"
                  :: "a"((uint32_t)val), "c"(xcr), "d"((uint32_t)(val >> 32))
                  : "memory");
}