/*
 * kernel/src/arch/riscv64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>

#include "asm/csr.h"
#include "asm/irqs.h"

#include "cpu/util.h"
#include "dev/syscon.h"
#include "lib/assert.h"

void cpu_idle() {
    while (true) {
        asm("wfi");
    }
}

void cpu_shutdown() {
    syscon_poweroff();
}

void cpu_reboot() {
    syscon_reboot();
}