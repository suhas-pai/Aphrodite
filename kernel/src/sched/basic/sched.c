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
    list_add(&g_run_queue, &current_thread()->sched_info.list);
}

__debug_optimize(3) void sched_enqueue_thread(struct thread *const thread) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);

    list_radd(&g_run_queue, &thread->sched_info.list);
    atomic_store_explicit(&thread->sched_info.runnable,
                          true,
                          memory_order_relaxed);

    spin_release_restore_irq(&g_run_queue_lock, flag);
}

__debug_optimize(3)
static void sched_dequeue_thread_for_use(struct thread *const thread) {
    list_remove(&thread->sched_info.list);
}

__debug_optimize(3) void sched_dequeue_thread(struct thread *const thread) {
    assert(thread != thread->cpu->idle_thread);
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);

    atomic_store_explicit(&thread->sched_info.runnable,
                          false,
                          memory_order_relaxed);

    sched_dequeue_thread_for_use(thread);
    spin_release_restore_irq(&g_run_queue_lock, flag);
}

struct cpu_info *sched_dequeue_current_thread() {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);

    struct thread *const thread = current_thread();
    struct cpu_info *const cpu = thread->cpu;

    atomic_store_explicit(&thread->sched_info.runnable,
                          false,
                          memory_order_relaxed);

    sched_dequeue_thread_for_use(thread);
    spin_release_restore_irq(&g_run_queue_lock, flag);

    return cpu;
}

__debug_optimize(3) bool thread_runnable(const struct thread *thread) {
    return atomic_load_explicit(&thread->sched_info.runnable,
                                memory_order_relaxed);
}

__debug_optimize(3) bool thread_enqueued(const struct thread *const thread) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);
    const bool result = !list_empty(&thread->sched_info.list);

    spin_release_restore_irq(&g_run_queue_lock, flag);
    return result;
}

extern void sched_set_current_thread(struct thread *thread);

__noinline
static struct thread *get_next_thread(struct thread *const prev) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);
    struct thread *next = NULL;

    list_foreach(next, &g_run_queue, sched_info.list) {
        if (next != prev) {
            sched_dequeue_thread_for_use(next);
            spin_release_restore_irq(&g_run_queue_lock, flag);

            return next;
        }
    }

    if (thread_runnable(prev)) {
        sched_dequeue_thread_for_use(prev);
        spin_release_restore_irq(&g_run_queue_lock, flag);

        return prev;
    }

    struct thread *const result = prev->cpu->idle_thread;
    spin_release_restore_irq(&g_run_queue_lock, flag);

    return result;
}

__noinline static void update_alarm_list(struct thread *const current_thread) {
    struct list *const alarm_list = &this_cpu_mut()->alarm_list;

    struct alarm *iter = NULL;
    struct alarm *tmp = NULL;

    list_foreach_mut(iter, tmp, alarm_list, list) {
        const usec_t time_spent =
            current_thread->sched_info.timeslice
          - current_thread->sched_info.remaining;

        if (iter->remaining > time_spent) {
            iter->remaining -= time_spent;
            continue;
        }

        atomic_store_explicit(&iter->active, false, memory_order_relaxed);
        sched_enqueue_thread(iter->listener);

        list_remove(&iter->list);
    }
}

extern __noreturn void thread_spinup(const struct thread_context *context);

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
        curr_thread->cpu = NULL;
    }

    sched_set_current_thread(next_thread);
    sched_irq_eoi(irq);

    sched_timer_oneshot(next_thread->sched_info.timeslice);
    sched_switch_to(/*prev=*/curr_thread, next_thread, context);

    verify_not_reached();
}

static void
yield(struct thread *const curr_thread, struct cpu_info *const cpu) {
    atomic_store_explicit(&curr_thread->sched_info.awaiting,
                          true,
                          memory_order_relaxed);

    curr_thread->sched_info.remaining = sched_timer_remaining();

    sched_timer_stop();
    sched_send_ipi(cpu);

    enable_interrupts();
    do {
        cpu_pause();
    } while (
        atomic_load_explicit(&curr_thread->sched_info.awaiting,
                             memory_order_relaxed));
}

void sched_yield() {
    disable_interrupts();
    assert(preemption_enabled());

    struct thread *const curr_thread = current_thread();
    yield(curr_thread, sched_dequeue_current_thread());
}

void sched_yield_with_this_cpu(struct cpu_info *const cpu) {
    disable_interrupts();
    assert(preemption_enabled());

    struct thread *const curr_thread = current_thread();
    yield(curr_thread, cpu);
}