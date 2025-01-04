/*
 * kernel/src/arch/loongarch64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>
#include <stddef.h>

#include "asm/irqs.h"
#include "cpu/util.h"
#include "lib/assert.h"

__noreturn void cpu_idle() {
    assert(intr_are_enabled());
    cpu_halt();
}

__noreturn void cpu_halt() {
    while (true) {
        asm volatile ("idle 0");
    }
}
