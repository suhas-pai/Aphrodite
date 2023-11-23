/*
 * kernel/src/time/time.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "lib/time.h"

struct clock_source {
    struct list list;
    const char *name;

    usec_t (*read)(struct clock_source *source);
    bool (*enable)(struct clock_source *source);
    bool (*disable)(struct clock_source *source);
    void (*resume)(struct clock_source *source);
    void (*suspend)(struct clock_source *source);
};

void arch_init_time();
void add_clock_source(struct clock_source *const clock);

uint64_t nsec_since_boot();