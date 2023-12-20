/*
 * kernel/src/sched/process.c
 * © suhas pai
 */

#include "process.h"

__hidden struct process kernel_process = {
    .pagemap = &kernel_pagemap,
    .threads = ARRAY_INIT(sizeof(struct thread *)),

    .name = SV_STATIC("kernel"),
    .pid = 0
};
