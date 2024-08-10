/*
 * kernel/src/arch/x86_64/sched/irq.c
 * © suhas pai
 */

#include "apic/lapic.h"
#include "cpu/isr.h"
#include "sched/scheduler.h"

__hidden isr_vector_t g_sched_vector = 0;

__debug_optimize(3) static void
sched_handle_irq(const uint64_t intr_no, struct thread_context *const frame) {
    lapic_timer_stop();
    sched_next(intr_no, frame);
}

__debug_optimize(3) void sched_init_irq() {
    g_sched_vector = isr_alloc_vector();
    assert(g_sched_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_sched_vector, sched_handle_irq, &ARCH_ISR_INFO_NONE());
}

__debug_optimize(3) isr_vector_t isr_get_sched_irq() {
    return g_sched_vector;
}

__debug_optimize(3) void sched_send_ipi(const struct cpu_info *const cpu) {
    lapic_send_ipi(cpu->lapic_id, g_sched_vector);
}

__debug_optimize(3) void sched_self_ipi() {
    lapic_send_self_ipi(g_sched_vector);
}
