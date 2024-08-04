/*
 * kernel/src/arch/loongarch64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>
#include <stddef.h>

#include "cpu/util.h"

void cpu_idle() {
    while (true) {
        asm volatile ("idle 0");
    }
}
