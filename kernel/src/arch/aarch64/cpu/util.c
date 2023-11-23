/*
 * kernel/src/arch/aarch64/cpu/util.c
 * © suhas pai
 */

#include "cpu/util.h"
#include "dev/psci.h"

void cpu_halt() {
    while (true) {
        asm volatile ("wfi");
    }
}

void cpu_shutdown() {
    psci_shutdown();
    cpu_halt();
}

void cpu_reboot() {
    psci_reboot();
    cpu_halt();
}