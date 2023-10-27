/*
 * kernel/arch/riscv64/sys/isr.c
 * © suhas pai
 */

#include "cpu/panic.h"
#include "sys/isr.h"

#include "cpu.h"

void isr_init() {

}

isr_vector_t isr_alloc_vector() {
    panic("isr: isr_alloc_vector() called, not supported on riscv64");
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    (void)vector;
    (void)handler;
    (void)info;

    panic("isr: isr_set_vector() but not implemented");
}

void
isr_assign_irq_to_cpu(struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    (void)cpu;
    (void)irq;
    (void)vector;
    (void)masked;

    panic("isr: isr_set_vector() but not implemented");
}