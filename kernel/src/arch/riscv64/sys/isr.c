/*
 * kernel/src/arch/riscv64/sys/isr.c
 * © suhas pai
 */

#include "dev/time/stime.h"

#include "asm/cause.h"
#include "asm/csr.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/printk.h"
#include "sched/scheduler.h"

static isr_func_t g_funcs[240] = {0};
static isr_vector_t g_free_vector = 16;

void isr_init() {

}

__optimize(3) isr_vector_t isr_alloc_vector(const bool for_msi) {
    (void)for_msi;

    const isr_vector_t result = g_free_vector;
    g_free_vector++;

    return result;
}

__optimize(3) void isr_mask_irq(const isr_vector_t irq) {
    (void)irq;
}

__optimize(3) void isr_unmask_irq(const isr_vector_t irq) {
    (void)irq;
}

extern
void handle_exception(const uint64_t vector, struct thread_context *frame);

__optimize(3) void
isr_handle_interrupt(const uint64_t cause,
                     const uint64_t epc,
                     struct thread_context *const frame)
{
    const isr_vector_t code = cause & __SCAUSE_CODE;
    if (cause & __SCAUSE_IS_INT) {
        switch ((enum cause_interrupt_kind)code) {
            case CAUSE_INTERRUPT_SUPERVISOR_IPI:
                return;
            case CAUSE_INTERRUPT_SUPERVISOR_TIMER:
                stimer_stop();
                sched_next(/*from_irq=*/true);

                return;
            case CAUSE_INTERRUPT_MACHINE_IPI:
            case CAUSE_INTERRUPT_MACHINE_TIMER:
                verify_not_reached();
        }

        if (g_funcs[code] != NULL) {
            g_funcs[code](code, epc, frame);
        } else {
            printk(LOGLEVEL_INFO,
                   "isr: got unhandled interrupt: " ISR_VECTOR_FMT "\n",
                   code);
        }
    } else {
        printk(LOGLEVEL_INFO,
               "exception:\n"
               "\tscause: " SV_FMT " (0x%" PRIx64 ")\n"
               "\tsepc: 0x%" PRIx64 "\n"
               "\tstval: 0x%" PRIx64 "\n\n",
               SV_FMT_ARGS(cause_exception_kind_get_sv(code)),
               cause,
               epc,
               csr_read(stval));

        cpu_idle();
    }
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

    panic("isr: isr_assign_irq_to_cpu() but not implemented");
}

__optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)cpu;
    (void)vector;

    verify_not_reached();
}

__optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)cpu;
    (void)vector;

    verify_not_reached();
}
