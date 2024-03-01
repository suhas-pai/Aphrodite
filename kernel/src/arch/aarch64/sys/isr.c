/*
 * kernel/src/arch/aarch64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitmap.h"

#include "asm/esr.h"
#include "asm/irqs.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/printk.h"
#include "sys/gic.h"

#define ISR_IRQ_COUNT 1020

struct irq_info {
    isr_func_t handler;

    bool alloced_in_msi : 1;
    bool for_msi : 1;
};

extern void *const ivt_el1;

static struct bitmap g_bitmap = BITMAP_INIT();
static struct irq_info g_irq_info_list[ISR_IRQ_COUNT] = {0};

__optimize(3) void isr_init() {
    g_bitmap =
        bitmap_alloc(distance_incl(GIC_SPI_INTERRUPT_START, ISR_IRQ_COUNT));
}

__optimize(3)
void isr_reserve_msi_irqs(const uint16_t base, const uint16_t count) {
    for (uint16_t irq = base; irq != base + count; irq++) {
        g_irq_info_list[irq].for_msi = true;

        gicd_set_irq_priority(irq, IRQ_POLARITY_HIGH);
        gicd_set_irq_trigger_mode(irq, IRQ_TRIGGER_MODE_EDGE);
    }

    bitmap_set_range(&g_bitmap, RANGE_INIT(base, count), /*value=*/true);
}

void isr_install_vbar() {
    asm volatile ("msr vbar_el1, %0" :: "r"(&ivt_el1));
    printk(LOGLEVEL_INFO, "isr: installed vbar_el1\n");
}

__optimize(3) isr_vector_t isr_alloc_vector(const bool for_msi) {
    if (for_msi) {
        struct gic_v2_msi_info *iter = NULL;
        struct list *const list = gicd_get_msi_info_list();

        list_foreach(iter, list, list) {
            assert(iter->initialized);

            const uint16_t end = iter->spi_base + iter->spi_count;
            for (uint16_t irq = iter->spi_base; irq != end; irq++) {
                assert(g_irq_info_list[irq].for_msi);
                if (!g_irq_info_list[irq].alloced_in_msi) {
                    g_irq_info_list[irq].alloced_in_msi = true;
                    return irq;
                }
            }
        }
    } else {
        const uint64_t result =
            bitmap_find(&g_bitmap,
                        /*count=*/1,
                        /*start_index=*/0,
                        /*expected_value=*/false,
                        /*value=*/true);

        if (result != FIND_BIT_INVALID) {
            return result;
        }
    }

    return ISR_INVALID_VECTOR;
}

__optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    (void)info;

    g_irq_info_list[vector].handler = handler;
    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
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

    verify_not_reached();
}

__optimize(3) void isr_mask_irq(const isr_vector_t irq) {
    gicd_mask_irq(irq);
}

__optimize(3) void isr_unmask_irq(const isr_vector_t irq) {
    gicd_unmask_irq(irq);
}

__optimize(3) void isr_eoi(const uint64_t intr_info) {
    gic_cpu_eoi(intr_info >> 16, /*irq_number=*/(uint16_t)intr_info);
}

__optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}

__optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}

__optimize(3) void handle_interrupt(struct thread_context *const context) {
    uint8_t cpu_id = 0;
    const irq_number_t irq = gic_cpu_get_irq_number(&cpu_id);

    if (__builtin_expect(irq >= ISR_IRQ_COUNT, 0)) {
        printk(LOGLEVEL_WARN,
               "isr: got spurious interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               irq,
               cpu_id);

        cpu_mut_for_intr_number(cpu_id)->spur_intr_count++;
        gic_cpu_eoi(cpu_id, irq);

        return;
    }

    const isr_func_t handler = g_irq_info_list[irq].handler;
    if (handler != NULL) {
        handler((uint64_t)cpu_id << 16 | irq, context);
    } else {
        printk(LOGLEVEL_WARN,
               "isr: got unhandled interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               irq,
               cpu_id);
    }
}

__optimize(3) void handle_sync_exception(struct thread_context *const context) {
    const uint64_t esr = context->esr_el1;
    enable_interrupts();

    const enum esr_error_code error_code =
        (esr & __ESR_ERROR_CODE) >> ESR_ERROR_CODE_SHIFT;

    switch (error_code) {
        case ESR_ERROR_CODE_UNKNOWN:
            printk(LOGLEVEL_WARN,
                   "isr: received a recognized unknown exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_WF:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped wf* instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC_EC0:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped mcrr/mrrc with ec 0 instruction "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_MCRR_OR_MRRC:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped mcrr/mrrc instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped mcr/mrc instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_LDC_OR_SDC:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped ldc/sdc instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_SVE:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped sve instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_LD64B_OR_SD64B:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped ld64b, st64b, st64bv, or st64bv0 "
                   "instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_MRRC:
            printk(LOGLEVEL_WARN,
                   "isr: received a trapped mrrc instruction exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_BRANCH_TARGET_EXCEPTION:
            printk(LOGLEVEL_WARN, "isr: received a branch target exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_ILLEGAL_EXEC_STATE:
            printk(LOGLEVEL_WARN, "isr: received an illegal exec exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SVC_IN_AARCH32:
            printk(LOGLEVEL_WARN, "isr: received a svc in aarch32 exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SVC_IN_AARCH64:
            printk(LOGLEVEL_WARN, "isr: received a svc in aarch64 exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_MSR_OR_MRS:
            printk(LOGLEVEL_WARN,
                   "isr: received a msr/mrs or other sys instruction "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TRAPPED_SVE_EC0:
            printk(LOGLEVEL_WARN, "isr: received a svc with ec 0 exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_TSTART_ACCESS:
            printk(LOGLEVEL_WARN, "isr: received a tstart access exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_PTR_AUTH_FAIL:
            printk(LOGLEVEL_WARN, "isr: received a ptr auth exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_INSTR_ABORT_LOWER_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received an instr abort from a lower el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_INSTR_ABORT_SAME_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received an instr abort from the same el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_PC_ALIGNMENT_FAULT:
            printk(LOGLEVEL_WARN,
                   "isr: received a pc alignment fault from a lower el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_DATA_ABORT_LOWER_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a data abort fault from a lower el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_DATA_ABORT_SAME_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a data abort fault from the same el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SP_ALIGNMENT_FAULT:
            printk(LOGLEVEL_WARN,
                   "isr: received a sp alignment fault from a lower el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_FP_ON_AARCH32_TRAP:
            printk(LOGLEVEL_WARN,
                   "isr: received a fp on aarch32 trap exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_FP_ON_AARCH64_TRAP:
            printk(LOGLEVEL_WARN,
                   "isr: received a fp on aarch64 trap exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SERROR_INTERRUPT:
            printk(LOGLEVEL_WARN,
                   "isr: received a serror trap exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_BREAKPOINT_LOWER_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a breakpoint from a lower el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_BREAKPOINT_SAME_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a breakpoint from the same el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SOFTWARE_STEP_LOWER_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a software step from a lowerl el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_SOFTWARE_STEP_SAME_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a software step from the same el "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_WATCHPOINT_LOWER_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a watchpoint from a lower el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_WATCHPOINT_SAME_EL:
            printk(LOGLEVEL_WARN,
                   "isr: received a watchpoint from the same el exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH32:
            printk(LOGLEVEL_WARN,
                   "isr: received a bkpt instrunction exec on aarch32 fault "
                   "exception\n");
            cpu_idle();
        case ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH64:
            printk(LOGLEVEL_WARN,
                   "isr: received a bkpt instrunction exec on aarch64 fault "
                   "exception\n");
            cpu_idle();
    }

    verify_not_reached();
}

__optimize(3)
static const char *aet_get_cstr(const enum esr_serror_aet_kind kind) {
    switch (kind) {
        case ESR_SERROR_AET_KIND_UNCONTAINBLE:
            return "uncontainable";
        case ESR_SERROR_AET_KIND_UNRECOVERABLE:
            return "unrecoverable";
        case ESR_SERROR_AET_KIND_RESTARTABLE:
            return "unrestartable";
        case ESR_SERROR_AET_KIND_RECOVERABLE:
            return "recoverable";
        case ESR_SERROR_AET_KIND_CORRECTED:
            return "corrected";
    }

    return "<unknown>";
}

void handle_async_exception(struct thread_context *const context) {
    const uint64_t esr = context->esr_el1;
    enable_interrupts();

    const enum esr_error_code error_code =
        (esr & __ESR_ERROR_CODE) >> ESR_ERROR_CODE_SHIFT;

    if (error_code != ESR_ERROR_CODE_SERROR_INTERRUPT) {
        printk(LOGLEVEL_WARN,
               "isr: received async exception w/o a serror error-code\n");
        return;
    }

    if (esr & __ESR_SERROR_IDS) {
        printk(LOGLEVEL_INFO,
               "isr: received async exception with impl-defined info\n");
        return;
    }

    const bool iesb = (esr & __ESR_SERROR_IESB) >> ESR_SERROR_IESB_SHIFT;
    const bool ext_abort =
        (esr & __ESR_SERROR_EXT_ABORT) >> ESR_SERROR_EXT_ABORT_SHIFT;

    const uint16_t dfsc = esr & __ESR_SERROR_DFSC;
    const enum esr_serror_aet_kind aet =
        (esr & __ESR_SERROR_AET) >> ESR_SERROR_AET_SHIFT;

    printk(LOGLEVEL_INFO,
           "isr: received async exception: %s%sserror\n"
           "\text-abort? %s\n"
           "\timplicit error sychronized? %s\n"
           "\tdata fault status code: 0x%" PRIx16 "\n",
           dfsc == ESR_SERROR_DFSC_KIND_ASYNC_ERROR ? aet_get_cstr(aet) : "",
           dfsc == ESR_SERROR_DFSC_KIND_ASYNC_ERROR ? " " : "",
           ext_abort ? "yes" : "no",
           iesb ? "yes" : "no",
           dfsc);
}

void handle_invalid_exception(struct thread_context *const context) {
    (void)context;
    panic("isr: got invalid exception\n");
}
