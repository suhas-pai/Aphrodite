/*
 * kernel/src/cpu/spinlock.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#if defined(DEBUG_LOCKS)
    #include "lib/assert.h"
#endif /* defined(DEBUG_LOCKS) */

#include "spinlock.h"

__optimize(3) void spin_acquire(struct spinlock *const lock) {
    const uint32_t ticket = atomic_fetch_add(&lock->back, 1);
    while (true) {
        if (atomic_load(&lock->front) == ticket) {
        #if defined(DEBUG_LOCKS)
            assert(lock->front <= lock->back);
        #endif /* defined(DEBUG_LOCKS) */

            return;
        }

        cpu_pause();
    }
}

__optimize(3) void spin_release(struct spinlock *const lock) {
    atomic_fetch_add(&lock->front, 1);

#if defined(DEBUG_LOCKS)
    assert(lock->front <= lock->back);
#endif /* defined(DEBUG_LOCKS) */
}

__optimize(3) bool spin_try_acquire(struct spinlock *const lock) {
    uint32_t ticket = atomic_load(&lock->back);
    const uint32_t front = atomic_load(&lock->front);

    if (front == ticket) {
        return atomic_compare_exchange_strong(&lock->back, &ticket, ticket + 1);
    }

    return false;
}

__optimize(3) int spin_acquire_with_irq(struct spinlock *const lock) {
    const bool irqs_enabled = are_interrupts_enabled();

    disable_interrupts();
    spin_acquire(lock);

    return irqs_enabled;
}

__optimize(3)
void spin_release_with_irq(struct spinlock *const lock, const int flag) {
    spin_release(lock);
    if (flag != 0) {
        enable_interrupts();
    }
}

__optimize(3) bool
spin_try_acquire_with_irq(struct spinlock *const lock, int *const flag_out) {
    const bool irqs_enabled = are_interrupts_enabled();
    disable_interrupts();

    if (!spin_try_acquire(lock)) {
        if (irqs_enabled) {
            enable_interrupts();
        }

        return false;
    }

    *flag_out = irqs_enabled;
    return true;
}