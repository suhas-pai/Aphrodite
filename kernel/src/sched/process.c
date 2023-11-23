/*
 * kernel/src/sched/process.c
 * © suhas pai
 */

#include "mm/pagemap.h"
#include "thread.h"

__hidden struct process kernel_process = {
    .pagemap = &kernel_pagemap,
    .threads = ARRAY_INIT(sizeof(struct thread *)),
    .name = "kernel"
};
