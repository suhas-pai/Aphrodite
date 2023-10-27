/*
 * kernel/arch/x86_64/init.c
 * © suhas pai
 */

#include "mm/init.h"

#include "sys/gdt.h"
#include "sys/idt.h"

#include "cpu.h"

void arch_early_init() {

}

void arch_init() {
    gdt_load();
    idt_init();
    cpu_init();
    mm_init();
}