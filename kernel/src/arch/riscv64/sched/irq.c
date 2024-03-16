/*
 * kernel/src/arch/riscv64/sched/irq.c
 * © suhas pai
 */

#include "cpu/info.h"
#include "lib/macros.h"
#include "sys/isr.h"

__hidden isr_vector_t g_sched_vector = 0;

void sched_init_irq() {

}

void sched_self_ipi() {

}

__optimize(3) void sched_send_ipi(const struct cpu_info *const cpu) {
    (void)cpu;
}

__optimize(3) isr_vector_t sched_get_isr_vector() {
    return g_sched_vector;
}