/*
 * kernel/src/sched/round_robin/init.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "sched/irq.h"
#include "sched/scheduler.h"
#include "sched/thread.h"
#include "sched/timer.h"

static struct list g_run_queue = LIST_INIT(g_run_queue);
static struct spinlock g_run_queue_lock = SPINLOCK_INIT();

void sched_process_algo_info_init(struct process *const process) {
    (void)process;
}

void sched_thread_algo_info_init(struct thread *const thread) {
    thread->sched_info.timeslice = SCHED_ROUND_ROBIN_DEF_TIMESLICE_US;
    thread->sched_info.awaiting = false;

    list_init(&thread->sched_info.list);
}

void sched_algo_init() {

}

__optimize(3) void sched_algo_post_init() {
    list_add(&g_run_queue, &current_thread()->sched_info.list);
}

__optimize(3) void sched_enqueue_thread(struct thread *const thread) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);

    list_radd(&g_run_queue, &thread->sched_info.list);
    atomic_store_explicit(&thread->sched_info.enqueued,
                          true,
                          memory_order_relaxed);

    spin_release_with_irq(&g_run_queue_lock, flag);
}

__optimize(3) void sched_dequeue_thread(struct thread *const thread) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);

    atomic_store_explicit(&thread->sched_info.enqueued,
                          false,
                          memory_order_relaxed);

    list_remove(&thread->sched_info.list);
    spin_release_with_irq(&g_run_queue_lock, flag);
}

__optimize(3) bool thread_enqueued(const struct thread *const thread) {
    return atomic_load_explicit(&thread->sched_info.enqueued,
                                memory_order_relaxed);
}

extern void sched_set_current_thread(struct thread *thread);

__optimize(3) static struct thread *get_next_thread(struct thread *const prev) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);
    struct thread *next = NULL;

    list_foreach(next, &g_run_queue, sched_info.list) {
        if (next != prev) {
            spin_release_with_irq(&g_run_queue_lock, flag);
            return next;
        }
    }

    if (thread_enqueued(prev)) {
        spin_release_with_irq(&g_run_queue_lock, flag);
        return prev;
    }

    spin_release_with_irq(&g_run_queue_lock, flag);
    return prev->cpu->idle_thread;
}

void sched_next(struct thread_context *const context, const bool from_irq) {
    struct thread *const curr_thread = current_thread();
    if (curr_thread->preemption_disabled) {
        if (from_irq) {
            sched_irq_eoi();
        }

        sched_timer_oneshot(curr_thread->sched_info.timeslice);
        return;
    }

    if (curr_thread->sched_info.awaiting) {
        curr_thread->sched_info.awaiting = false;
    }

    struct thread *const next_thread = get_next_thread(curr_thread);

    // Only possible if we were idle and will still be idle, or if our current
    // thread is still runnable and no other thread is.

    if (curr_thread == next_thread) {
        if (from_irq) {
            sched_irq_eoi();
        }

        sched_timer_oneshot(curr_thread->sched_info.timeslice);
        return;
    }

    const struct thread *const idle_thread = this_cpu()->idle_thread;
    if (next_thread == idle_thread) {
        sched_set_current_thread(next_thread);
        if (from_irq) {
            sched_irq_eoi();
        }

        sched_timer_oneshot(next_thread->sched_info.timeslice);
        sched_switch_to(/*prev=*/curr_thread, next_thread, context, from_irq);

        verify_not_reached();
    }

    next_thread->cpu = curr_thread->cpu;
    if (curr_thread != idle_thread) {
        curr_thread->cpu = NULL;
    }

    sched_prepare_thread(next_thread);
    sched_set_current_thread(next_thread);

    if (from_irq) {
        sched_irq_eoi();
    }

    sched_timer_oneshot(next_thread->sched_info.timeslice);
    sched_switch_to(/*prev=*/curr_thread, next_thread, context, from_irq);
}

__optimize(3) void sched_yield() {
    disable_interrupts();
    sched_timer_stop();

    struct thread *const curr_thread = current_thread();

    assert(!curr_thread->preemption_disabled);
    curr_thread->sched_info.awaiting = true;

    sched_send_ipi(curr_thread->cpu);
    enable_interrupts();

    do {
        cpu_pause();
    } while (
        atomic_load_explicit(&curr_thread->sched_info.awaiting,
                             memory_order_relaxed));
}
