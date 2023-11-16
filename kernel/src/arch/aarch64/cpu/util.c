/*
 * kernel/src/arch/aarch64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>
#include "cpu/util.h"

void cpu_halt() {
    while (true) {
        asm("wfi");
    }
}