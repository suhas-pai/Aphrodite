/*
 * kernel/src/time/time.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "lib/math.h"

#include "clock.h"

static struct list g_clock_list = LIST_INIT(g_clock_list);
static struct spinlock g_lock = SPINLOCK_INIT();

void clock_add(struct clock *const clock) {
    const int flag = spin_acquire_with_irq(&g_lock);

    list_add(&g_clock_list, &clock->list);
    spin_release_with_irq(&g_lock, flag);
}

uint64_t
clock_read_in_res(const struct clock *const clock,
                  const enum clock_resolution resolution)
{
    if (clock->resolution == resolution) {
        return clock->read(clock);
    }

    if (resolution > clock->resolution) {
        const uint64_t diff = resolution - clock->resolution;
        return clock->read(clock) / math_pow_assert(1000, diff);
    }

    const uint64_t diff = clock->resolution - resolution;
    return clock->read(clock) * math_pow_assert(1000, diff);
}