/*
 * kernel/src/arch/aarch64/cpu/util.c
 * © suhas pai
 */

#include "cpu/panic.h"
#include "cpu/util.h"

#include "dev/psci.h"

void cpu_idle() {
    while (true) {
        asm volatile ("wfi");
    }
}

void cpu_shutdown() {
    const enum psci_return_value result = psci_shutdown();
    panic("kernel: cpu_shutdown() failed with result=%d\n", result);
}

void cpu_reboot() {
    const enum psci_return_value result = psci_reboot();
    panic("kernel: cpu_reboot() failed with result=%d\n", result);
}