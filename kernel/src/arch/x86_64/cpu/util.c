/*
 * kernel/src/arch/x86_64/cpu/util.c
 * © suhas pai
 */

#include <lib/assert.h>

#include "asm/irqs.h"
#include "cpu/util.h"

[[noreturn]] void cpu_idle() {
    assert(intr_are_enabled());
    cpu_halt();
}

[[noreturn]] void cpu_halt() {
    while (true) {
        asm("hlt");
    }
}
