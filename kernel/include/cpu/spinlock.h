/*
 * kernel/include/cpu/spinlock.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lib/thread_safety.h>

struct CAPABILITY("spinlock") spinlock {
    _Atomic uint32_t front;
    _Atomic uint32_t back;
};

#define SPINLOCK_INIT() \
    ((struct spinlock){ \
        .front = 0, \
        .back = 0 \
    })

#define with_spinlock_acquired(lock, block) \
    do { \
        spin_acquire(lock); \
        block; \
        spin_release(lock); \
    } while (false)

#define with_spinlock_intr_disabled(lock, block) \
    do { \
        const int h_var(spin_irq_flag) = spin_acquire_save_intr(lock); \
        block; \
        spin_release_restore_intr((lock), h_var(spin_irq_flag)); \
    } while (false)

#define with_spinlock_preempt_disabled(lock, block) \
    do { \
        spin_acquire_preempt_disable(lock); \
        block; \
        spin_release_preempt_enable(lock); \
    } while (false)

void spin_acquire(struct spinlock *lock);
void spin_release(struct spinlock *lock);

int spin_acquire_save_intr(struct spinlock *lock);
void spin_acquire_preempt_disable(struct spinlock *lock);

void spin_release_restore_intr(struct spinlock *lock, int flag);
void spin_release_preempt_enable(struct spinlock *lock);

bool spin_try_acquire(struct spinlock *lock);
bool spin_try_acquire_save_intr(struct spinlock *lock, int *flag_out);

void spinlock_deinit(struct spinlock *lock);
