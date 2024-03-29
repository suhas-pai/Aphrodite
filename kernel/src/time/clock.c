/*
 * kernel/src/time/time.c
 * © suhas pai
 */

#include "cpu/spinlock.h"

#include "lib/math.h"
#include "lib/overflow.h"

#include "clock.h"

static struct list g_clock_list = LIST_INIT(g_clock_list);
static struct spinlock g_lock = SPINLOCK_INIT();

void clock_add(struct clock *const clock) {
    const int flag = spin_acquire_irq_save(&g_lock);

    list_add(&g_clock_list, &clock->list);
    spin_release_irq_restore(&g_lock, flag);
}

bool
clock_read_res(const struct clock *const clock,
               const enum clock_resolution resolution,
               uint64_t *const result_out)
{
    if (clock->resolution == resolution) {
        *result_out = clock->read(clock);
        return true;
    }

    if (resolution > clock->resolution) {
        const uint64_t diff = resolution - clock->resolution;
        *result_out = clock->read(clock) / math_pow_assert(1000, diff);

        return true;
    }

    const uint64_t diff = clock->resolution - resolution;
    return check_mul(clock->read(clock),
                     math_pow_assert(1000, diff),
                     result_out);
}