/*
 * kernel/src/arch/riscv64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>

#include "asm/irqs.h"
#include "cpu/util.h"
#include "dev/syscon.h"
#include "lib/assert.h"

__noreturn void cpu_idle() {
    assert(intr_are_enabled());
    cpu_halt();
}

__noreturn void cpu_halt() {
    while (true) {
        asm("wfi");
    }
}

__noreturn void cpu_shutdown() {
    syscon_poweroff();
}

__noreturn void cpu_reboot() {
    syscon_reboot();
}