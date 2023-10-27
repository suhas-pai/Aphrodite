/*
 * kernel/arch/aarch64/init.c
 * © suhas pai
 */

#include "mm/init.h"
#include "sys/isr.h"

#include "cpu.h"

void arch_early_init() {

}

void arch_init() {
    cpu_init();
    mm_init();

    isr_install_vbar();
}