/*
 * kernel/src/cpu/spinlock.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct spinlock {
    _Atomic uint32_t front;
    _Atomic uint32_t back;
};

#define SPINLOCK_INIT() \
    ((struct spinlock){ \
        .front = 0, \
        .back = 0 \
    })

void spin_acquire(struct spinlock *lock);
void spin_release(struct spinlock *lock);

int spin_acquire_save_irq(struct spinlock *lock);
void spin_acquire_preempt_disable(struct spinlock *lock);

void spin_release_restore_irq(struct spinlock *lock, int flag);
void spin_release_preempt_enable(struct spinlock *lock);

bool spin_try_acquire(struct spinlock *lock);
bool spin_try_acquire_save_irq(struct spinlock *lock, int *flag_out);

void spinlock_deinit(struct spinlock *lock);
