/*
 * kernel/src/arch/riscv64/dev/time/stime.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "asm/irqs.h"

#include "cpu/info.h"
#include "lib/time.h"
#include "sched/thread.h"

__debug_optimize(3) usec_t stime_get() {
    const usec_t ticks_per_micro =
        get_cpus_info()->timebase_frequency / MICRO_IN_SECONDS;

    return csr_read(time) / ticks_per_micro;
}

__debug_optimize(3) void stimer_oneshot(const usec_t interval) {
    csr_clear(sie, __INTR_SUPERVISOR_TIMER);
    const usec_t ticks_per_micro =
        get_cpus_info()->timebase_frequency / MICRO_IN_SECONDS;

    const uint64_t ticks = check_mul_assert(interval, ticks_per_micro);
    const usec_t time = csr_read(time);

    csr_write(stimecmp, time + ticks);
    with_preempt_disabled({
        this_cpu_mut()->timer_start = time;
    });

    csr_set(sie, __INTR_SUPERVISOR_TIMER);
}

__debug_optimize(3) void stimer_stop() {
    csr_clear(sie, __INTR_SUPERVISOR_TIMER);
    csr_write(stimecmp, 0);
}