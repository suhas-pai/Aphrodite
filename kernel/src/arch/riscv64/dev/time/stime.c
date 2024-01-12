/*
 * kernel/src/arch/riscv64/dev/time/stime.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "asm/irqs.h"

#include "cpu/info.h"
#include "lib/time.h"

__optimize(3) usec_t stime_get() {
    const usec_t ticks_per_micro =
        get_cpus_info()->timebase_frequency / MICRO_IN_SECONDS;

    return csr_read(time) / ticks_per_micro;
}

__optimize(3) void stimer_oneshot(const usec_t interval) {
    csr_clear(sie, __IE_SUPERVISOR_TIMER_INT_ENABLE);

    const usec_t ticks_per_micro =
        get_cpus_info()->timebase_frequency / MICRO_IN_SECONDS;

    const uint64_t ticks = check_mul_assert(interval, ticks_per_micro);

    csr_write(stimecmp, csr_read(time) + ticks);
    csr_set(sie, __IE_SUPERVISOR_TIMER_INT_ENABLE);
}

__optimize(3) void stimer_stop() {
    csr_clear(sie, __IE_SUPERVISOR_TIMER_INT_ENABLE);
    csr_write(stimecmp, 0);
}