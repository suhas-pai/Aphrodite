/*
 * kernel/src/arch/aarch64/sys/isr.c
 * © suhas pai
 */

#include <lib/adt/bitset.h>

#include "sys/gic/api.h"
#include "sys/gic/its.h"

#include "asm/esr.h"
#include "asm/irqs.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/printk.h"

#define ISR_IRQ_COUNT 1020

struct intr_info {
    isr_func_t handler;
    void *ctx;

    bool alloced_in_msi : 1;
    bool for_msi : 1;
};

extern void *const ivt_el1;
static bitset_decl(g_vector_bitset, ISR_IRQ_COUNT - GIC_SPI_INTR_START);

// irqs correspond to spi intrs on arm

static struct irq_pin
g_irq_pin_list[GIC_SPI_INTR_LAST - GIC_SPI_INTR_START] = {0};

static struct intr_info g_irq_info_list[ISR_IRQ_COUNT] = {0};
static struct intr_info g_lpi_irq_info_list[GIC_ITS_MAX_LPIS_SUPPORTED] = {0};

static struct spinlock g_sgi_lock = SPINLOCK_INIT();
static uint16_t g_sgi_interrupt = 0;

__debug_optimize(3) void isr_init() {

}

__debug_optimize(3) isr_vector_t isr_alloc_sgi_vector() {
    uint16_t result = 0;
    with_spinlock_intr_disabled(&g_sgi_lock, {
        result = g_sgi_interrupt;
    });

    if (result > GIC_SGI_INTR_LAST) {
        return ISR_INVALID_VECTOR;
    }

    g_sgi_interrupt++;
    return result;
}

__debug_optimize(3)
void isr_reserve_msi_irqs(const uint16_t base, const uint16_t count) {
    for (uint16_t irq = base; irq != base + count; irq++) {
        g_irq_info_list[irq].for_msi = true;

        gicd_set_irq_priority(irq, IRQ_POLARITY_HIGH);
        gicd_set_irq_trigger_mode(irq, IRQ_TRIGGER_MODE_EDGE);

        bitset_set(g_vector_bitset, irq);
    }
}

void isr_install_vbar() {
    asm volatile ("msr vbar_el1, %0" :: "r"(&ivt_el1));
    printk(LOGLEVEL_INFO, "isr: installed vbar_el1\n");
}

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    const uint64_t result =
        bitset_find_unset(g_vector_bitset, ISR_IRQ_COUNT, /*invert=*/true);

    if (result != BITSET_INVALID) {
        return GIC_SPI_INTR_START + result;
    }

    return ISR_INVALID_VECTOR;
}

__debug_optimize(3) isr_vector_t
isr_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    return gicd_alloc_msi_vector(device, msi_index);
}

__debug_optimize(3) void isr_free_vector(const isr_vector_t vector) {
    assert_msg(bitset_has(g_vector_bitset, vector),
               "isr: isr_free_vector() called on unallocated vector");

    bitset_unset(g_vector_bitset, /*invert=*/true);
}

__debug_optimize(3) void
isr_free_msi_vector(struct device *const device,
                    const isr_vector_t vector,
                    const uint16_t msi_index)
{
    gicd_free_msi_vector(device, vector, msi_index);
}

__debug_optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               void *const ctx,
               struct arch_isr_info *const info)
{
    (void)info;

    if (vector >= GIC_ITS_LPI_INTERRUPT_START) {
        const uint16_t index = vector - GIC_ITS_LPI_INTERRUPT_START;
        if (!index_in_bounds(index, GIC_ITS_MAX_LPIS_SUPPORTED)) {
            printk(LOGLEVEL_WARN,
                   "isr: isr_set_vector() called on invalid lpi\n");
            return;
        }

        g_lpi_irq_info_list[index].handler = handler;
        g_lpi_irq_info_list[index].ctx = ctx;
    } else {
        g_irq_info_list[vector].handler = handler;
        g_irq_info_list[vector].ctx = ctx;
    }

    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
}

__debug_optimize(3) void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   void *const ctx,
                   struct arch_isr_info *const info)
{
    (void)info;
    if (!index_in_bounds(vector, GIC_ITS_MAX_LPIS_SUPPORTED)) {
        printk(LOGLEVEL_WARN,
               "isr: isr_set_vector() called on invalid lpi\n");
        return;
    }

    g_lpi_irq_info_list[vector].handler = handler;
    g_lpi_irq_info_list[vector].ctx = ctx;

    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
}

struct irq_pin *isr_get_irq_pin(const irq_number_t irq) {
    if (!index_in_bounds(irq, countof(g_irq_pin_list))) {
        return nullptr;
    }

    return &g_irq_pin_list[irq];
}

bool
isr_install_irq(struct irq_pin *const pin,
                const isr_func_t handler,
                void *const ctx,
                const bool masked)
{
    isr_set_vector(pin->irq, handler, ctx, &ARCH_ISR_INFO_NONE());
    if (masked) {
        isr_mask_irq(pin);
    }

    return true;
}

void *isr_uninstall_irq(struct irq_pin *const pin) {
    void *result = nullptr;
    if (pin->vector >= GIC_ITS_LPI_INTERRUPT_START) {
        const uint16_t index = pin->vector - GIC_ITS_LPI_INTERRUPT_START;
        result = g_lpi_irq_info_list[index].ctx;
    } else {
        result = g_irq_info_list[pin->vector].ctx;
    }

    isr_free_vector(pin->vector);
    pin->vector = ISR_INVALID_VECTOR;

    return result;
}

__debug_optimize(3) void isr_mask_intr(const isr_vector_t intr) {
    gicd_mask_irq(intr);
}

__debug_optimize(3) void isr_unmask_intr(const isr_vector_t intr) {
    gicd_unmask_irq(intr);
}

__debug_optimize(3) void isr_mask_irq(struct irq_pin *const pin) {
    isr_mask_intr(pin->irq);
}

__debug_optimize(3) void isr_unmask_irq(struct irq_pin *const pin) {
    isr_unmask_intr(pin->irq);
}

__debug_optimize(3) void isr_eoi(const uint64_t intr_info) {
    gic_cpu_eoi(intr_info >> 16, /*irq_number=*/(uint16_t)intr_info);
}

__debug_optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}

__debug_optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}

__debug_optimize(3) enum isr_msi_support isr_get_msi_support() {
    return gicd_get_msi_support();
}

__debug_optimize(3)
void handle_interrupt(struct thread_context *const context) {
    uint8_t cpu_id = 0;
    const irq_number_t intr = gic_cpu_get_irq_number(&cpu_id);

    this_cpu_mut()->called_eoi = false;
    if (intr >= GIC_ITS_LPI_INTERRUPT_START) {
        const uint16_t index = intr - GIC_ITS_LPI_INTERRUPT_START;
        this_cpu_mut()->in_lpi = true;

        if (!index_in_bounds(index, GIC_ITS_MAX_LPIS_SUPPORTED)) {
            printk(LOGLEVEL_WARN,
                   "isr: got lpi beyond end of supported lpis\n");

            gic_cpu_eoi(cpu_id, intr);
            return;
        }

        const isr_func_t handler = g_lpi_irq_info_list[index].handler;
        void *const ctx = g_lpi_irq_info_list[index].ctx;

        if (handler != nullptr) {
            handler((uint64_t)cpu_id << 16 | index, context, ctx);
            if (!this_cpu_mut()->called_eoi) {
                printk(LOGLEVEL_WARN,
                       "isr: lpi handler for irq " IRQ_NUMBER_FMT ", "
                       "lpi %" PRIu16 " did not call eoi\n",
                       intr,
                       index);

                gic_cpu_eoi(cpu_id, intr);
            }
        } else {
            printk(LOGLEVEL_WARN,
                   "isr: got unhandled lpi interrupt " ISR_VECTOR_FMT " on "
                   "cpu %" PRIu8 "\n",
                   intr,
                   cpu_id);

            gic_cpu_eoi(cpu_id, intr);
        }

        return;
    }

    if (__builtin_expect(intr >= ISR_IRQ_COUNT, 0)) {
        printk(LOGLEVEL_WARN,
               "isr: got spurious interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               intr,
               cpu_id);

        cpu_for_id_mut(cpu_id)->spur_intr_count++;
        gic_cpu_eoi(cpu_id, intr);

        return;
    }

    const isr_func_t handler = g_irq_info_list[intr].handler;
    void *const ctx = g_irq_info_list[intr].ctx;

    if (handler != nullptr) {
        handler((uint64_t)cpu_id << 16 | intr, context, ctx);
        if (!this_cpu_mut()->called_eoi) {
            printk(LOGLEVEL_WARN,
                   "isr: lpi handler for irq " IRQ_NUMBER_FMT "did not call "
                   "eoi\n",
                   intr);

            gic_cpu_eoi(cpu_id, intr);
        }
    } else {
        printk(LOGLEVEL_WARN,
               "isr: got unhandled interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               intr,
               cpu_id);

        gic_cpu_eoi(cpu_id, intr);
    }
}

__debug_optimize(3)
void handle_sync_exception(struct thread_context *const context) {
    this_cpu_mut()->in_exception = true;

    const uint64_t esr = context->esr_el1;
    const enum esr_error_code error_code =
        (esr & __ESR_ERROR_CODE) >> ESR_ERROR_CODE_SHIFT;

    printk(LOGLEVEL_ERROR,
           "isr: received sync exception\n"
           "\telr_el1: %p\n"
           "\tfar_el1: %p\n"
           "\tsp_el1: %p\n",
           (void *)context->elr_el1,
           (void *)context->far_el1,
           (void *)context->sp_el1);

    switch (error_code) {
        case ESR_ERROR_CODE_UNKNOWN:
            printk(LOGLEVEL_ERROR, "kind: unknown\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_WF:
            printk(LOGLEVEL_ERROR, "kind: trapped wf* instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC_EC0:
            printk(LOGLEVEL_ERROR,
                   "kind: trapped mcrr/mrrc with ec0 instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_MCRR_OR_MRRC:
            printk(LOGLEVEL_ERROR, "kind: trapped mcrr/mrrc instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC:
            printk(LOGLEVEL_ERROR, "kind: trapped mcr/mrc instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_LDC_OR_SDC:
            printk(LOGLEVEL_ERROR, "kind: trapped ldc/sdc instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_SVE:
            printk(LOGLEVEL_ERROR, "kind: trapped sve instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_LD64B_OR_SD64B:
            printk(LOGLEVEL_ERROR,
                   "kind: trapped ld64b, st64b, st64bv, or st64bv0 "
                   "instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_MRRC:
            printk(LOGLEVEL_ERROR, "kind: trapped mrrc instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_BRANCH_TARGET_EXCEPTION:
            printk(LOGLEVEL_ERROR, "kind: branch target\n");
            cpu_halt();
        case ESR_ERROR_CODE_ILLEGAL_EXEC_STATE:
            printk(LOGLEVEL_ERROR, "kind: illegal exec\n");
            cpu_halt();
        case ESR_ERROR_CODE_SVC_IN_AARCH32:
            printk(LOGLEVEL_ERROR, "kind: svc in aarch32\n");
            cpu_halt();
        case ESR_ERROR_CODE_SVC_IN_AARCH64:
            printk(LOGLEVEL_ERROR, "kind: svc in aarch64\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_MSR_OR_MRS:
            printk(LOGLEVEL_ERROR, "kind: msr/mrs or other sys instruction\n");
            cpu_halt();
        case ESR_ERROR_CODE_TRAPPED_SVE_EC0:
            printk(LOGLEVEL_ERROR, "kind: svc with ec 0\n");
            cpu_halt();
        case ESR_ERROR_CODE_TSTART_ACCESS:
            printk(LOGLEVEL_ERROR, "kind: tstart access\n");
            cpu_halt();
        case ESR_ERROR_CODE_PTR_AUTH_FAIL:
            printk(LOGLEVEL_ERROR, "kind: ptr auth\n");
            cpu_halt();
        case ESR_ERROR_CODE_INSTR_ABORT_LOWER_EL:
            printk(LOGLEVEL_ERROR, "kind: instr abort from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_INSTR_ABORT_SAME_EL:
            printk(LOGLEVEL_ERROR, "kind: instr abort from the same el\n");
            cpu_halt();
        case ESR_ERROR_CODE_PC_ALIGNMENT_FAULT:
            printk(LOGLEVEL_ERROR,
                   "kind: pc alignment fault from a lower el\n");
            cpu_halt();
        case ESR_ERROR_PAGE_TABLE_WALK_EL1:
            printk(LOGLEVEL_ERROR, "kind: page table walk error at el1\n");
            cpu_halt();
        case ESR_ERROR_CODE_DATA_ABORT_LOWER_EL:
            printk(LOGLEVEL_ERROR, "kind: data abort fault from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_DATA_ABORT_SAME_EL:
            printk(LOGLEVEL_ERROR,
                   "kind: data abort fault from the same el\n");
            cpu_halt();
        case ESR_ERROR_CODE_SP_ALIGNMENT_FAULT:
            printk(LOGLEVEL_ERROR,
                   "kind: sp alignment fault from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_FP_ON_AARCH32_TRAP:
            printk(LOGLEVEL_ERROR, "kind: fp on aarch32 trap\n");
            cpu_halt();
        case ESR_ERROR_CODE_FP_ON_AARCH64_TRAP:
            printk(LOGLEVEL_ERROR, "kind: fp on aarch64 trap\n");
            cpu_halt();
        case ESR_ERROR_CODE_SERROR_INTERRUPT:
            printk(LOGLEVEL_ERROR, "kind: serror trap\n");
            cpu_halt();
        case ESR_ERROR_CODE_BREAKPOINT_LOWER_EL:
            printk(LOGLEVEL_ERROR, "kind: breakpoint from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_BREAKPOINT_SAME_EL:
            printk(LOGLEVEL_ERROR, "kind: breakpoint from the same el\n");
            cpu_halt();
        case ESR_ERROR_CODE_SOFTWARE_STEP_LOWER_EL:
            printk(LOGLEVEL_ERROR, "kind: software step from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_SOFTWARE_STEP_SAME_EL:
            printk(LOGLEVEL_ERROR, "kind: software step from the same el\n");
            cpu_halt();
        case ESR_ERROR_CODE_WATCHPOINT_LOWER_EL:
            printk(LOGLEVEL_ERROR, "kind: watchpoint from a lower el\n");
            cpu_halt();
        case ESR_ERROR_CODE_WATCHPOINT_SAME_EL:
            printk(LOGLEVEL_ERROR, "kind: watchpoint from the same el\n");
            cpu_halt();
        case ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH32:
            printk(LOGLEVEL_ERROR,
                   "kind: bkpt instruction exec on aarch32 fault\n");
            cpu_halt();
        case ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH64:
            printk(LOGLEVEL_ERROR,
                   "kind: bkpt instruction exec on aarch64 fault\n");
            cpu_halt();
    }

    printk(LOGLEVEL_WARN,
           "\tunknown synchronous exception\n"
           "\t\tcode: %d\n",
           error_code);

    cpu_halt();
}

__debug_optimize(3)
static const char *aet_get_cstr(const enum esr_serror_aet_kind kind) {
    switch (kind) {
        case ESR_SERROR_AET_KIND_UNCONTAINABLE:
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
    intr_disable();

    const enum esr_error_code error_code =
        (esr & __ESR_ERROR_CODE) >> ESR_ERROR_CODE_SHIFT;

    if (error_code != ESR_ERROR_CODE_SERROR_INTERRUPT) {
        printk(LOGLEVEL_WARN,
               "isr: received async exception w/o a serror error-code\n");
        cpu_halt();
    }

    if (esr & __ESR_SERROR_IDS) {
        printk(LOGLEVEL_WARN,
               "isr: received async exception with impl-defined info\n");
        cpu_halt();
    }

    const bool iesb = (esr & __ESR_SERROR_IESB) >> ESR_SERROR_IESB_SHIFT;
    const bool ext_abort =
        (esr & __ESR_SERROR_EXT_ABORT) >> ESR_SERROR_EXT_ABORT_SHIFT;

    const uint16_t dfsc = esr & __ESR_SERROR_DFSC;
    const enum esr_serror_aet_kind aet =
        (esr & __ESR_SERROR_AET) >> ESR_SERROR_AET_SHIFT;

    printk(LOGLEVEL_ERROR,
           "isr: received async exception: %s%sserror\n"
           "\text-abort? %s\n"
           "\timplicit error synchronized? %s\n"
           "\tdata fault status code: 0x%" PRIx16 "\n",
           dfsc == ESR_SERROR_DFSC_KIND_ASYNC_ERROR ? aet_get_cstr(aet) : "",
           dfsc == ESR_SERROR_DFSC_KIND_ASYNC_ERROR ? " " : "",
           ext_abort ? "yes" : "no",
           iesb ? "yes" : "no",
           dfsc);

    cpu_halt();
}

void handle_invalid_exception(struct thread_context *const context) {
    (void)context;
    panic("isr: got invalid exception\n");
}
