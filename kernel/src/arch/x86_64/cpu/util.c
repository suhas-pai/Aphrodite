/*
 * kernel/src/arch/x86_64/cpu/util.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/util.h"
#include "lib/assert.h"

[[noreturn]] void cpu_idle() {
    assert(intr_are_enabled());
    cpu_halt();
}

[[noreturn]] void cpu_halt() {
    while (true) {
        asm("hlt");
    }
}
