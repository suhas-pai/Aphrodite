/*
 * kernel/dev/time/time.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "time.h"

static struct list g_clock_list = LIST_INIT(g_clock_list);
static struct spinlock g_lock = SPINLOCK_INIT();

void add_clock_source(struct clock_source *const clock) {
    const int flag = spin_acquire_with_irq(&g_lock);

    list_add(&g_clock_list, &clock->list);
    spin_release_with_irq(&g_lock, flag);
}