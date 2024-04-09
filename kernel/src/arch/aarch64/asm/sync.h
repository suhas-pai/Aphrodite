/*
 * kernel/src/arch/aarch64/asm/dsb.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__optimize(3) static inline void dsbisht() {
    asm volatile("dsb ishst");
}

__optimize(3) static inline void dsb() {
    asm volatile("dsb sy");
}

__optimize(3) static inline void isb() {
    asm volatile("isb");
}
