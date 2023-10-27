/*
 * kernel/arch/riscv64/init.c
 * © suhas pai
 */

#include "mm/init.h"

void arch_early_init() {

}

void arch_init() {
    mm_init();
}