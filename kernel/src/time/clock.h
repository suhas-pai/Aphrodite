/*
 * kernel/src/time/time.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string_view.h"

#include "lib/list.h"
#include "lib/time.h"

enum clock_resolution {
    CLOCK_RES_SECONDS,
    CLOCK_RES_MILLI,
    CLOCK_RES_MICRO,
    CLOCK_RES_NANO,
    CLOCK_RES_PICO,
    CLOCK_RES_FEMTO,
};

struct clock {
    struct list list;
    struct string_view name;

    enum clock_resolution resolution : 3;
    bool one_shot_capable : 1;

    time_t (*read)(const struct clock *clock);

    bool (*enable)(const struct clock *clock);
    bool (*disable)(const struct clock *clock);

    void (*oneshot)(const struct clock *clock, nsec_t interval);
};

bool
clock_read_res(const struct clock *clock,
               enum clock_resolution res,
               uint64_t *result_out);

__optimize(3) static inline bool
clock_has_atleast_res(const struct clock *const clock,
                      const enum clock_resolution res)
{
    return clock->resolution >= res;
}

void clock_add(struct clock *clock);

struct clock *system_clock_get();
struct clock *rtc_clock_get();
