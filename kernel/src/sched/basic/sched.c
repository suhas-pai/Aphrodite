/*
 * kernel/src/sched/round_robin/init.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "mm/kmalloc.h"

#include "sched/alarm.h"
#include "sched/irq.h"
#include "sched/scheduler.h"
#include "sched/timer.h"

static struct list g_run_queue = LIST_INIT(g_run_queue);
static struct spinlock g_run_queue_lock = SPINLOCK_INIT();

void sched_process_algo_info_init(struct process *const process) {
    (void)process;
}

void sched_thread_algo_info_init(struct thread *const thread) {
    thread->sched_info.timeslice = SCHED_BASIC_DEF_TIMESLICE_US;
    thread->sched_info.awaiting = false;
    thread->sched_info.runnable = false;

    list_init(&thread->sched_info.list);
}

void sched_algo_init() {

}

__debug_optimize(3) void sched_algo_post_init() {
    kernel_main_thread.sched_info.runnable = true;
}

__debug_optimize(3) bool thread_runnable(const struct thread *const thread) {
    return atomic_load_explicit(&thread->sched_info.runnable,
                                memory_order_relaxed);
}

__debug_optimize(3)
bool is_on_runlist_nolock(const struct thread *const thread) {
    return !list_empty(&thread->sched_info.list);
}

__debug_optimize(3) bool thread_enqueued(const struct thread *const thread) {
    bool result = false;
    with_spinlock_intr_disabled(&g_run_queue_lock, {
        result = is_on_runlist_nolock(thread);
    });

    return result;
}

__debug_optimize(3) bool is_running_nolock(const struct thread *const thread) {
    return thread->cpu != nullptr;
}

__debug_optimize(3) bool thread_running(const struct thread *const thread) {
    bool result = false;
    with_spinlock_intr_disabled(&g_run_queue_lock, {
        result = is_running_nolock(thread);
    });

    return result;
}

__debug_optimize(3)
static void sched_add_to_run_list(struct thread *const thread) {
    list_radd(&g_run_queue, &thread->sched_info.list);
}

__debug_optimize(3)
static void sched_remove_from_run_list(struct thread *const thread) {
    list_remove(&thread->sched_info.list);
}

__debug_optimize(3) void sched_wake(struct thread *const thread) {
    with_spinlock_intr_disabled(&g_run_queue_lock, {
        // sched_enqueue_thread() might be called from a dequeued but running
        // thread so only enqueue-for-use for threads that are not running and
        // aren't already enqueued.

        if (!is_running_nolock(thread) && !is_on_runlist_nolock(thread)) {
            sched_add_to_run_list(thread);
        }

        atomic_store_explicit(&thread->sched_info.runnable,
                              true,
                              memory_order_relaxed);
    });
}

__debug_optimize(3) void sched_dequeue_thread(struct thread *const thread) {
    with_spinlock_intr_disabled(&g_run_queue_lock, {
        atomic_store_explicit(&thread->sched_info.runnable,
                               false,
                               memory_order_relaxed);

        sched_remove_from_run_list(thread);
    });
}

__debug_optimize(3)
static struct thread *get_next_thread(struct thread *const prev) {
    const int flag = spin_acquire_save_intr(&g_run_queue_lock);
    struct thread *next = nullptr;

    list_foreach(&g_run_queue, sched_info.list, next) {
        if (next == prev) {
            continue;
        }

        if (thread_runnable(prev)) {
            sched_add_to_run_list(prev);
        }

        sched_remove_from_run_list(next);
        spin_release_restore_intr(&g_run_queue_lock, flag);

        return next;
    }

    if (thread_runnable(prev)) {
        spin_release_restore_intr(&g_run_queue_lock, flag);
        return prev;
    }

    struct cpu_info *const cpu = prev->cpu;
    struct thread *const result = cpu->idle_thread;

    spin_release_restore_intr(&g_run_queue_lock, flag);
    return result;
}

__debug_optimize(3)
static void update_alarm_list(struct thread *const current_thread) {
    struct list *const alarm_list = &this_cpu_mut()->alarm_list;

    struct alarm *iter = nullptr;
    struct alarm *tmp = nullptr;

    list_foreach_mut(alarm_list, list, iter, tmp) {
        const usec_t time_spent =
            current_thread->sched_info.timeslice -
            current_thread->sched_info.remaining;

        if (iter->remaining > time_spent) {
            iter->remaining -= time_spent;
            continue;
        }

        iter->callback(iter, iter->ctx);

        atomic_store_explicit(&iter->triggered, true, memory_order_relaxed);
        atomic_store_explicit(&iter->posted, true, memory_order_relaxed);

        list_remove(&iter->list);
    }
}

void sched_set_current_thread(struct thread *thread);
[[noreturn]] extern void thread_spinup(const struct thread_context *context);

void sched_next(const irq_number_t irq, struct thread_context *const context) {
    kmalloc_check_slabs();

    struct thread *const curr_thread = current_thread();
    thread_context_verify(curr_thread->process, context);

    if (curr_thread->preemption_disabled > 0) {
        sched_irq_eoi(irq);
        sched_timer_oneshot(curr_thread->sched_info.timeslice);

        return;
    }

    curr_thread->sched_info.awaiting = false;
    curr_thread->sched_info.remaining = 0;

    update_alarm_list(curr_thread);
    struct thread *const next_thread = get_next_thread(curr_thread);

    // Only possible if we were idle and will still be idle, or if our current
    // thread is still runnable and no other thread is.

    if (curr_thread == next_thread) {
        sched_irq_eoi(irq);
        sched_timer_oneshot(curr_thread->sched_info.timeslice);

        return;
    }

    struct cpu_info *const cpu = curr_thread->cpu;
    const struct thread *const idle_thread = cpu->idle_thread;

    next_thread->cpu = cpu;
    if (curr_thread != idle_thread) {
        curr_thread->cpu = nullptr;
    }

    sched_set_current_thread(next_thread);
    sched_save_restore_context(/*prev=*/curr_thread, next_thread, context);

    if (curr_thread->process != next_thread->process) {
        switch_to_pagemap(&next_thread->process->pagemap);
    }

    sched_irq_eoi(irq);
    sched_timer_oneshot(next_thread->sched_info.timeslice);

    thread_spinup(&next_thread->context);
    verify_not_reached();
}

void sched_yield() {
    intr_disable();
    assert(preemption_enabled());

    struct thread *const curr_thread = current_thread();
    atomic_store_explicit(&curr_thread->sched_info.awaiting,
                          true,
                          memory_order_relaxed);

    curr_thread->sched_info.remaining = sched_timer_remaining();

    sched_timer_stop();
    sched_self_ipi();

    intr_enable();

    do {
        cpu_pause();
    } while (
        atomic_load_explicit(&curr_thread->sched_info.awaiting,
                             memory_order_relaxed));
}

void sched_await() {
    sched_dequeue_thread(current_thread());
    sched_yield();
}