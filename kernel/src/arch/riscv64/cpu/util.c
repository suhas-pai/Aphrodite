/*
 * kernel/src/arch/riscv64/cpu/util.c
 * © suhas pai
 */

#include <stdbool.h>

#include "cpu/util.h"
#include "dev/syscon.h"

void cpu_halt() {
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