/*
 * kernel/src/arch/riscv64/sched/irq.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/isr.h"
#include "sched/scheduler.h"
#include "sys/mmio.h"

__hidden isr_vector_t g_sched_sgi_vector = 0;

__debug_optimize(3) static void
ipi_handler(const uint64_t intr_no, struct thread_context *const context) {
    sched_next(intr_no, context);
}

void sched_init_irq() {
    g_sched_sgi_vector = isr_alloc_msi_vector(/*device=*/NULL, /*msi_index=*/0);
    isr_set_msi_vector(g_sched_sgi_vector, ipi_handler, &ARCH_ISR_INFO_NONE());
}

void sched_self_ipi() {
    with_irqs_disabled({
        mmio_write(&this_cpu()->imsic_page[0], g_sched_sgi_vector);
    });
}

__debug_optimize(3) void sched_send_ipi(const struct cpu_info *const cpu) {
    mmio_write(&cpu->imsic_page[0], g_sched_sgi_vector);
}