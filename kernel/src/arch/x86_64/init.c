/*
 * kernel/src/arch/x86_64/init.c
 * © suhas pai
 */

#include "apic/lapic.h"
#include "asm/irqs.h"

#include "cpu/init.h"
#include "mm/init.h"

#include "sched/scheduler.h"

#include "sys/gdt.h"
#include "sys/idt.h"

void arch_early_init() {

}

void arch_post_mm_init() {

}

__debug_optimize(3) void arch_init_for_smp(struct limine_smp_info *const cpu) {
    cpu_init_for_smp(cpu);

    gdt_load();
    idt_load();
    lapic_init();

    switch_to_pagemap(&kernel_process.pagemap);
    assert(!are_interrupts_enabled());

    sched_yield();
    verify_not_reached();
}

__debug_optimize(3) void arch_init() {
    gdt_load();
    idt_init();
    cpu_init();
    mm_arch_init();
}