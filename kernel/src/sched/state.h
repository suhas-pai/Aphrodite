/*
 * kernel/src/sched/state.h
 * © suhas pai
 */

#pragma once

enum thread_state {
    THREAD_STATE_NONE,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_RUNNABLE,
    THREAD_STATE_RUNNING,
};
