/*
 * kernel/src/arch/loongarch64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "cpu/isr.h"
#include "cpu/spinlock.h"

#define ISR_IRQ_COUNT 2
#define ISR_MSI_COUNT 256

static bitset_decl(g_msi_bitset, ISR_MSI_COUNT);

static struct spinlock g_lock = SPINLOCK_INIT();

static isr_func_t g_funcs[ISR_IRQ_COUNT] = {0};
static isr_func_t g_msi_funcs[ISR_MSI_COUNT] = {0};

void isr_init() {

}

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    const int flag = spin_acquire_save_irq(&g_lock);
    if (g_funcs[0] == NULL) {
        spin_release_restore_irq(&g_lock, flag);
        return 0;
    }

    if (g_funcs[1] == NULL) {
        spin_release_restore_irq(&g_lock, flag);
        return 1;
    }

    spin_release_restore_irq(&g_lock, flag);
    return ISR_INVALID_VECTOR;
}

__debug_optimize(3) isr_vector_t
isr_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    (void)device;
    (void)msi_index;

    uint64_t result = 0;
    SPIN_WITH_IRQ_ACQUIRED(&g_lock, {
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
    SPIN_WITH_IRQ_ACQUIRED(&g_lock, {
        g_funcs[vector] = NULL;
    });
}

__debug_optimize(3) void
isr_free_msi_vector(struct device *const device,
                    const isr_vector_t vector,
                    const uint16_t msi_index)
{
    (void)device;
    (void)msi_index;

    SPIN_WITH_IRQ_ACQUIRED(&g_lock, {
        bitset_unset(g_msi_bitset, vector);
        isr_set_vector(vector, /*handler=*/NULL, &ARCH_ISR_INFO_NONE());
    });
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    (void)info;
    g_funcs[vector] = handler;
}

void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   struct arch_isr_info *const info)
{
    (void)info;
    g_msi_funcs[vector] = handler;
}

void isr_eoi(const uint64_t intr_info) {
    (void)intr_info;
}

void
isr_assign_irq_to_cpu(const struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    (void)cpu;
    (void)irq;
    (void)vector;
    (void)masked;
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
