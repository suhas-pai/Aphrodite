/*
 * kernel/src/arch/aarch64/sched/timer.c
 * © suhas pai
 */

#include "dev/time/time.h"
#include "sys/gic/v2.h"

#include "cpu/info.h"
#include "sched/irq.h"

void sched_timer_oneshot(const usec_t usec) {
    system_timer_oneshot_ns(micro_to_nano(usec));
}

void sched_timer_stop() {
    system_timer_stop_alarm();
}

void sched_irq_eoi() {
    gic_cpu_eoi(this_cpu()->interface_number, sched_get_isr_vector());
}

__optimize(3) usec_t sched_timer_remaining() {
    return nano_to_micro(system_timer_get_remaining_ns());
}