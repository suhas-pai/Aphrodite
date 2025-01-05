/*
 * kernel/src/arch/loongarch64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "cpu/isr.h"
#include "cpu/spinlock.h"

#include "sys/irq.h"
#include "sys/isr.h"

#define ISR_IRQ_COUNT 2
#define ISR_MSI_COUNT 256

static bitset_decl(g_msi_bitset, ISR_MSI_COUNT);
static struct spinlock g_lock = SPINLOCK_INIT();

struct intr_callback {
    isr_func_t handler;
    void *ctx;
};

static struct irq_pin g_irq_pin_list[ISR_IRQ_COUNT] = {0};
static struct intr_callback g_funcs[ISR_IRQ_COUNT] = {0};
static struct intr_callback g_msi_funcs[ISR_MSI_COUNT] = {0};

void isr_init() {

}

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    isr_vector_t result = ISR_INVALID_VECTOR;
    with_spinlock_intr_disabled(&g_lock, {
        if (g_funcs[0].handler == nullptr) {
            result = 0;
        } else if (g_funcs[1].handler == nullptr) {
            result = 1;
        }
    });

    return result;
}

__debug_optimize(3) isr_vector_t
isr_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    (void)device;
    (void)msi_index;

    uint64_t result = 0;
    with_spinlock_intr_disabled(&g_lock, {
        result =
            bitset_find_unset(g_msi_bitset, ISR_MSI_COUNT, /*invert=*/true);
    });

    if (result == BITSET_INVALID) {
        return ISR_INVALID_VECTOR;
    }

    return (isr_vector_t)result;
}

__debug_optimize(3) void isr_free_vector(const isr_vector_t vector) {
    assert(vector < 2);
    with_spinlock_intr_disabled(&g_lock, {
        g_funcs[vector].handler = nullptr;
        g_funcs[vector].ctx = nullptr;
    });
}

__debug_optimize(3) void
isr_free_msi_vector(struct device *const device,
                    const isr_vector_t vector,
                    const uint16_t msi_index)
{
    (void)device;
    (void)msi_index;

    with_spinlock_intr_disabled(&g_lock, {
        bitset_unset(g_msi_bitset, vector);
        isr_set_vector(vector,
                       /*handler=*/nullptr,
                       /*ctx=*/nullptr,
                       &ARCH_ISR_INFO_NONE());
    });
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               void *const vtx,
               struct arch_isr_info *const info)
{
    (void)info;

    g_funcs[vector].handler = handler;
    g_funcs[vector].ctx = vtx;
}

void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   void *const ctx,
                   struct arch_isr_info *const info)
{
    (void)info;

    g_msi_funcs[vector].handler = handler;
    g_msi_funcs[vector].ctx = ctx;
}

void isr_eoi(const uint64_t intr_info) {
    (void)intr_info;
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

void *isr_uninstall_irq(struct irq_pin *const pin) {
    void *const result = g_funcs[pin->irq].ctx;

    isr_free_vector(pin->vector);
    pin->vector = ISR_INVALID_VECTOR;

    return result;
}

__debug_optimize(3) void isr_mask_intr(const isr_vector_t intr) {
    (void)intr;
}

__debug_optimize(3) void isr_unmask_intr(const isr_vector_t intr) {
    (void)intr;
}

__debug_optimize(3) void isr_mask_irq(struct irq_pin *const pin) {
    isr_unmask_intr(pin->irq);
}

__debug_optimize(3) void isr_unmask_irq(struct irq_pin *const pin) {
    isr_unmask_intr(pin->irq);
}

__debug_optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)cpu;
    (void)vector;

    verify_not_reached();
}

__debug_optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)cpu;
    (void)vector;

    verify_not_reached();
}

__debug_optimize(3) enum isr_msi_support isr_get_msi_support() {
    return ISR_MSI_SUPPORT_NONE;
}
