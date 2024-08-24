/*
 * kernel/src/arch/x86_64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>

#include "asm/irqs.h"
#include "cpu/util.h"
#include "lib/assert.h"

__noreturn void cpu_idle() {
    assert(are_interrupts_enabled());
    cpu_halt();
}

__noreturn void cpu_halt() {
    while (true) {
        asm("hlt");
    }
}
