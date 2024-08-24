/*
 * kernel/src/cpu/spinlock.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "sched/thread.h"

#if defined(DEBUG_LOCKS)
    #include "lib/assert.h"
#endif /* defined(DEBUG_LOCKS) */

#include "spinlock.h"

__debug_optimize(3) void spin_acquire(struct spinlock *const lock) {
    const uint32_t ticket = atomic_fetch_add(&lock->back, 1);
    while (true) {
        const uint32_t front = atomic_load(&lock->front);
        if (front == ticket) {
        #if defined(DEBUG_LOCKS)
            assert(front <= atomic_load(&lock->back));
        #endif /* defined(DEBUG_LOCKS) */

            return;
        }

        cpu_pause();
    }
}

__debug_optimize(3) void spin_release(struct spinlock *const lock) {
    atomic_fetch_add(&lock->front, 1);
}

__debug_optimize(3) bool spin_try_acquire(struct spinlock *const lock) {
    uint32_t ticket = atomic_load(&lock->back);
    const uint32_t front = atomic_load(&lock->front);

    if (front == ticket) {
        return atomic_compare_exchange_strong(&lock->back, &ticket, ticket + 1);
    }

    return false;
}

__debug_optimize(3) int spin_acquire_save_irq(struct spinlock *const lock) {
    const bool irqs_enabled = are_interrupts_enabled();
    if (irqs_enabled) {
        disable_interrupts();
    }

    spin_acquire(lock);
    return irqs_enabled;
}

void spin_acquire_preempt_disable(struct spinlock *const lock) {
    preempt_disable();
    spin_acquire(lock);
}

__debug_optimize(3)
void spin_release_restore_irq(struct spinlock *const lock, const int flag) {
    spin_release(lock);
    if (flag != 0) {
        enable_interrupts();
    }
}

void spin_release_preempt_enable(struct spinlock *const lock) {
    spin_release(lock);
    preempt_enable();
}

__debug_optimize(3) bool
spin_try_acquire_save_irq(struct spinlock *const lock, int *const flag_out) {
    const bool irqs_enabled = disable_irqs_if_enabled();
    if (!spin_try_acquire(lock)) {
        enable_irqs_if_flag(irqs_enabled);
        return false;
    }

    *flag_out = irqs_enabled;
    return true;
}

__debug_optimize(3) void spinlock_deinit(struct spinlock *const lock) {
#if defined(DEBUG_LOCKS)
    assert(atomic_load(&lock->front) == atomic_load(&lock->back));
#endif /* defined(DEBUG_LOCKS) */

    lock->front = 0;
    lock->back = 0;
}
