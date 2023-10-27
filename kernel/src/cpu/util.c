/*
 * kernel/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>
#include "util.h"

void cpu_halt() {
    while (true) {
    #if defined (__x86_64__)
        asm("hlt");
    #elif defined (__aarch64__) || defined (__riscv64)
        asm("wfi");
    #endif
    }
}