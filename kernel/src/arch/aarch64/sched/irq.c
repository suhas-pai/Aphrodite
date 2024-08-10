/*
 * kernel/src/arch/aarch64/sched/irq.c
 * © suhas pai
 */

#include "sys/gic/api.h"

#include "cpu/isr.h"
#include "sched/scheduler.h"

__hidden isr_vector_t g_sched_sgi_vector = 0;

__debug_optimize(3) static void
sched_handle_irq(const uint64_t intr_no, struct thread_context *const frame) {
    sched_next(intr_no, frame);
}

__debug_optimize(3) void sched_init_irq() {
    g_sched_sgi_vector = isr_alloc_sgi_vector();
    assert(g_sched_sgi_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_sched_sgi_vector, sched_handle_irq, &ARCH_ISR_INFO_NONE());
}

__debug_optimize(3) void sched_self_ipi() {

}

__debug_optimize(3) isr_vector_t sched_get_isr_sgi_vector() {
    return g_sched_sgi_vector;
}

__debug_optimize(3) void sched_send_ipi(const struct cpu_info *const cpu) {
    gicd_send_ipi(cpu, sched_get_isr_sgi_vector());
}
