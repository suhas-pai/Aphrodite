/*
 * kernel/src/arch/x86_64/init.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "apic/lapic.h"

#include "cpu/init.h"
#include "cpu/smp.h"
#include "cpu/util.h"

#include "mm/init.h"
#include "sched/scheduler.h"

#include "sys/gdt.h"
#include "sys/idt.h"

void arch_early_init() {

}

void arch_post_mm_init() {

}

void sched_set_current_thread(struct thread *thread);

__debug_optimize(3) void arch_init_for_smp(struct limine_smp_info *const info) {
    cpu_init_for_smp();

    gdt_load();
    idt_load();

    struct smp_boot_info *const smp_info =
        (struct smp_boot_info *)info->extra_argument;

    struct cpu_info *const cpu = smp_info->cpu;

    sched_set_current_thread(cpu->idle_thread);
    switch_to_pagemap(&kernel_process.pagemap);

    lapic_init();
    atomic_store_explicit(&smp_info->booted, true, memory_order_seq_cst);

    // On cpu init, we point current-thread to the idle-thread, so we have to
    // call cpu_idle() for the rare case where the scheduler has no threads for
    // us to run, so it returns execution to us.

    sched_yield();
    cpu_idle();

    verify_not_reached();
}

__debug_optimize(3) void arch_init() {
    gdt_load();
    idt_init();
    cpu_init();
    mm_arch_init();
}