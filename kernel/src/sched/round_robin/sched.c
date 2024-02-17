/*
 * kernel/src/sched/round_robin/init.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/util.h"

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
    thread->sched_info.state = SCHED_THREAD_INFO_STATE_READY;
    thread->sched_info.timeslice = SCHED_ROUND_ROBIN_DEF_TIMESLICE_US;

    list_init(&thread->sched_info.list);
}

void sched_algo_init() {

}

__optimize(3) void sched_enqueue_thread(struct thread *const thread) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);

    list_add(&g_run_queue, &thread->sched_info.list);
    thread->sched_info.state = SCHED_THREAD_INFO_STATE_READY;

    spin_release_with_irq(&g_run_queue_lock, flag);
}

__optimize(3) void sched_dequeue_thread(struct thread *const thread) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);

    list_remove(&thread->sched_info.list);
    thread->sched_info.state = SCHED_THREAD_INFO_STATE_ASLEEP;

    spin_release_with_irq(&g_run_queue_lock, flag);
}

extern void sched_set_current_thread(struct thread *thread);

__optimize(3) static struct thread *get_next_thread(struct thread *const prev) {
    const int flag = spin_acquire_with_irq(&g_run_queue_lock);
    struct thread *next = NULL;

    list_foreach(next, &prev->sched_info.list, sched_info.list) {
        if (&next->sched_info.list == &g_run_queue) {
            next->sched_info.state = SCHED_THREAD_INFO_STATE_READY;

            list_remove(&next->sched_info.list);
            spin_release_with_irq(&g_run_queue_lock, flag);

            return next;
        }
    }

    if (prev->sched_info.state == SCHED_THREAD_INFO_STATE_READY) {
        prev->sched_info.state = SCHED_THREAD_INFO_STATE_RUNNING;

        list_remove(&next->sched_info.list);
        spin_release_with_irq(&g_run_queue_lock, flag);

        return prev;
    }

    spin_release_with_irq(&g_run_queue_lock, flag);
    return prev->cpu->idle_thread;
}

void sched_next(const bool from_irq) {
    struct thread *const curr_thread = current_thread();
    if (curr_thread->premption_disabled) {
        if (from_irq) {
            sched_irq_eoi();
        }

        sched_timer_oneshot(curr_thread->sched_info.timeslice);
        return;
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

    next_thread->cpu = this_cpu_mut();
    if (next_thread == this_cpu()->idle_thread) {
        sched_set_current_thread(next_thread);
        if (from_irq) {
            sched_irq_eoi();
        }

        sched_timer_oneshot(next_thread->sched_info.timeslice);
        sched_switch_to(/*prev=*/curr_thread, next_thread, from_irq);

        return;
    }

    next_thread->sched_info.state = SCHED_THREAD_INFO_STATE_RUNNING;

    sched_prepare_thread(next_thread);
    sched_set_current_thread(next_thread);

    sched_timer_oneshot(next_thread->sched_info.timeslice);
    sched_switch_to(/*prev=*/curr_thread, next_thread, from_irq);
}

__optimize(3) void sched_yield(const bool noreturn) {
    disable_interrupts();
    sched_timer_stop();

    spin_acquire(&g_run_queue_lock);

    current_thread()->sched_info.state = SCHED_THREAD_INFO_STATE_ASLEEP;
    spin_release(&g_run_queue_lock);

    enable_interrupts();
    sched_next(/*from_irq=*/false);

    if (noreturn) {
        sched_set_current_thread(this_cpu()->idle_thread);
        cpu_idle();
    }
}