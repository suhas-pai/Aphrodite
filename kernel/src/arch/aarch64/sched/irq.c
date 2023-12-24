/*
 * kernel/src/arch/aarch64/sched/irq.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/isr.h"

__hidden isr_vector_t g_sched_vector = 0;

void sched_init_irq() {

}