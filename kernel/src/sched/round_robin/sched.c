/*
 * kernel/src/sched/round_robin/init.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "dev/printk.h"
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
    thread->sched_info.timeslice = SCHED_ROUND_ROBIN_DEF_TIMESLICE_US;
    thread->sched_info.awaiting = false;
    thread->sched_info.enqueued = true;

    list_init(&thread->sched_info.list);
}

void sched_algo_init() {

}

__optimize(3) void sched_algo_post_init() {
    list_add(&g_run_queue, &current_thread()->sched_info.list);
}

__optimize(3) void sched_enqueue_thread(struct thread *const thread) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);

    list_radd(&g_run_queue, &thread->sched_info.list);
    atomic_store_explicit(&thread->sched_info.enqueued,
                          true,
                          memory_order_relaxed);

    spin_release_restore_irq(&g_run_queue_lock, flag);
}

__optimize(3) void sched_dequeue_thread(struct thread *const thread) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);

    assert(thread != thread->cpu->idle_thread);
    atomic_store_explicit(&thread->sched_info.enqueued,
                          false,
                          memory_order_relaxed);

    list_remove(&thread->sched_info.list);
    spin_release_restore_irq(&g_run_queue_lock, flag);
}

__optimize(3) bool thread_enqueued(const struct thread *const thread) {
    return atomic_load_explicit(&thread->sched_info.enqueued,
                                memory_order_relaxed);
}

extern void sched_set_current_thread(struct thread *thread);

__optimize(3)
static struct thread *get_next_thread(struct thread *const prev) {
    const int flag = spin_acquire_save_irq(&g_run_queue_lock);
    struct thread *next = NULL;

    list_foreach(next, &g_run_queue, sched_info.list) {
        if (next != prev) {
            spin_release_restore_irq(&g_run_queue_lock, flag);
            return next;
        }
    }

    if (thread_enqueued(prev)) {
        spin_release_restore_irq(&g_run_queue_lock, flag);
        return prev;
    }

    struct thread *const result = prev->cpu->idle_thread;
    spin_release_restore_irq(&g_run_queue_lock, flag);

    return result;
}

__optimize(3)
static void update_alarm_list(struct thread *const current_thread) {
    int flag = 0;

    struct alarm *iter = NULL;
    struct alarm *tmp = NULL;

    struct list *const alarm_list = alarm_get_list_locked(&flag);
    list_foreach_mut(iter, tmp, alarm_list, list) {
        const usec_t time_spent =
            current_thread->sched_info.timeslice
            - current_thread->sched_info.remaining;

        if (iter->remaining > time_spent) {
            iter->remaining -= time_spent;
            continue;
        }

        iter->remaining = 0;

        atomic_store_explicit(&iter->active, false, memory_order_relaxed);
        sched_enqueue_thread(iter->listener);

        list_remove(&iter->list);
    }

    alarm_list_unlock(flag);
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void sched_next(const irq_number_t irq, struct thread_context *const context) {
    kmalloc_check_slabs();

    struct thread *const curr_thread = current_thread();
    if (curr_thread->preemption_disabled != 0) {
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

void sched_yield() {
    disable_interrupts();
    assert(preemption_enabled());

    struct thread *const curr_thread = current_thread();

    curr_thread->sched_info.awaiting = true;
    curr_thread->sched_info.remaining = sched_timer_remaining();

    sched_timer_stop();
    sched_send_ipi(curr_thread->cpu);

    enable_interrupts();
    do {
        cpu_pause();
    } while (
        atomic_load_explicit(&curr_thread->sched_info.awaiting,
                             memory_order_relaxed));
}
