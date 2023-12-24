/*
 * kernel/src/time/time.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string_view.h"

#include "lib/list.h"
#include "lib/time.h"

enum clock_resolution {
    CLOCK_RES_FEMTO,
    CLOCK_RES_PICO,
    CLOCK_RES_NANO,
    CLOCK_RES_MICRO,
    CLOCK_RES_MILLI,
    CLOCK_RES_SECONDS,
};

struct clock {
    struct list list;
    struct string_view name;

    enum clock_resolution resolution : 3;

    time_t (*read)(const struct clock *source);
    bool (*enable)(const struct clock *clock);
    bool (*disable)(const struct clock *clock);
    void (*resume)(const struct clock *clock);
    void (*suspend)(const struct clock *clock);
};

uint64_t
clock_read_in_res(const struct clock *clock, enum clock_resolution res);

void clock_add(struct clock *const clock);

struct clock *system_clock_get();
struct clock *rtc_clock_get();

nsec_t nsec_since_boot();