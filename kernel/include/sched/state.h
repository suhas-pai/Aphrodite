/*
 * kernel/src/sched/state.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum thread_state : uint8_t {
    THREAD_STATE_NONE,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_RUNNABLE,
    THREAD_STATE_RUNNING,
};
