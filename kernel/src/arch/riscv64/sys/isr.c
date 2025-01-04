/*
 * kernel/src/arch/riscv64/sys/isr.c
 * © suhas pai
 */

#include "dev/time/stime.h"
#include "lib/adt/bitset.h"

#include "asm/cause.h"
#include "asm/csr.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/printk.h"
#include "sched/scheduler.h"

#include "sys/imsic.h"
#include "sys/irq.h"

#define ISR_IRQ_COUNT 240

static bitset_decl(g_vector_bitset, ISR_IRQ_COUNT);
static struct irq_pin g_irq_pin_list[ISR_IRQ_COUNT];

struct intr_callback {
    isr_func_t handler;
    void *ctx;
};

static struct spinlock g_lock = SPINLOCK_INIT();
static struct intr_callback g_callbacks[ISR_IRQ_COUNT] = {0};

void isr_init() {

}

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    uint64_t result = 0;
    with_spinlock_intr_disabled(&g_lock, {
        result =
            bitset_find_unset(g_vector_bitset, ISR_IRQ_COUNT, /*invert=*/true);
    });

    if (result == BITSET_INVALID) {
        return ISR_INVALID_VECTOR;
    }

    return (isr_vector_t)result;
}

__debug_optimize(3) isr_vector_t
isr_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    (void)device;
    (void)msi_index;

    return imsic_alloc_msg(RISCV64_PRIVL_SUPERVISOR);
}

__debug_optimize(3) void isr_free_vector(const isr_vector_t vector) {
    with_spinlock_intr_disabled(&g_lock, {
        bitset_unset(g_vector_bitset, vector);
        isr_set_vector(vector,
                       /*handler=*/nullptr,
                       /*ctx=*/nullptr,
                       &ARCH_ISR_INFO_NONE());
    });
}

__debug_optimize(3) void
isr_free_msi_vector(struct device *const device,
                    const isr_vector_t vector,
                    const uint16_t msi_index)
{
    (void)device;
    (void)msi_index;

    imsic_free_msg(RISCV64_PRIVL_SUPERVISOR, vector);
}

__debug_optimize(3) void isr_mask_intr(const isr_vector_t intr) {
    (void)intr;
}

__debug_optimize(3) void isr_unmask_intr(const isr_vector_t intr) {
    (void)intr;
}

__debug_optimize(3) void isr_mask_irq(struct irq_pin *const pin) {
    isr_mask_intr(pin->irq);
}

__debug_optimize(3) void isr_unmask_irq(struct irq_pin *const pin) {
    isr_mask_intr(pin->irq);
}

void isr_eoi(const uint64_t int_no) {
    (void)int_no;

    const uint8_t code = this_cpu()->isr_code;
    this_cpu_mut()->isr_code = 0;

    csr_write(sip, rm_mask(csr_read(sip), 1ull << code));
}

extern
void handle_exception(const uint64_t vector, struct thread_context *frame);

__debug_optimize(3) void
isr_handle_interrupt(const uint64_t cause, struct thread_context *const context)
{
    const isr_vector_t code = cause & __SCAUSE_CODE;
    this_cpu_mut()->isr_code = code;

    if ((cause & __SCAUSE_IS_INTR) == 0) {
        this_cpu_mut()->in_exception = true;
        printk(LOGLEVEL_INFO,
               "exception:\n"
               "\tscause: " SV_FMT " (0x%" PRIx64 ")\n"
               "\tsepc: 0x%" PRIx64 "\n"
               "\tstval: 0x%" PRIx64 "\n\n",
               SV_FMT_ARGS(cause_exception_kind_get_sv(code)),
               cause,
               context->sepc,
               csr_read(stval));

        cpu_idle();
    }

    switch ((enum cause_interrupt_kind)code) {
        case CAUSE_INTERRUPT_MACHINE_SW_INTR:
            panic("Got machine software interrupt");
        case CAUSE_INTERRUPT_USER_SW_INTR:
            panic("Got user software interrupt");
        case CAUSE_INTERRUPT_SUPERVISOR_SW_INTR:
            return;
        case CAUSE_INTERRUPT_USER_TIMER:
            panic("Got user timer interrupt");
        case CAUSE_INTERRUPT_SUPERVISOR_TIMER:
            stimer_stop();
            sched_next(code, context);

            return;
        case CAUSE_INTERRUPT_MACHINE_TIMER:
            panic("Got machine timer interrupt");
        case CAUSE_INTERRUPT_USER_EXTERNAL:
            panic("Got user external interrupt");
        case CAUSE_INTERRUPT_SUPERVISOR_EXTERNAL:
            imsic_handle(RISCV64_PRIVL_SUPERVISOR, context);
            return;
        case CAUSE_INTERRUPT_MACHINE_EXTERNAL:
            panic("Got machine external interrupt");
    }

    if (g_callbacks[code].handler != nullptr) {
        g_callbacks[code].handler(code, context, g_callbacks[code].ctx);
        return;
    }

    printk(LOGLEVEL_INFO,
           "isr: got unhandled interrupt: " ISR_VECTOR_FMT "\n",
           code);

    cpu_idle();
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               void *const ctx,
               struct arch_isr_info *const info)
{
    (void)info;
    imsic_set_msg_handler(RISCV64_PRIVL_SUPERVISOR, vector, handler, ctx);
}

void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   void *const ctx,
                   struct arch_isr_info *const info)
{
    (void)info;

    imsic_set_msg_handler(RISCV64_PRIVL_SUPERVISOR, vector, handler, ctx);
    imsic_enable_msg(RISCV64_PRIVL_SUPERVISOR, vector);
}

struct irq_pin *isr_get_irq_pin(const uint16_t irq) {
    assert(index_in_bounds(irq, countof(g_irq_pin_list)));
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

void isr_uninstall_irq(struct irq_pin *const pin) {
    isr_free_vector(pin->vector);
    pin->vector = ISR_INVALID_VECTOR;
}

__debug_optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)vector;
    return cpu->imsic_phys;
}

__debug_optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)vector;
    return cpu->imsic_phys;
}

__debug_optimize(3) enum isr_msi_support isr_get_msi_support() {
    return ISR_MSI_SUPPORT_MSIX;
}
