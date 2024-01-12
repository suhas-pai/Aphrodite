/*
 * kernel/src/sched/process.c
 * © suhas pai
 */

#include "process.h"

__hidden struct process kernel_process = {
    .pagemap = &kernel_pagemap,
    .threads = ARRAY_INIT(sizeof(struct thread *)),

    .name = STRING_STATIC("kernel"),
    .pid = 0
};

void
sched_process_init(struct process *const process,
                   struct pagemap *const pagemap,
                   const struct string name)
{
    process->pagemap = pagemap;
    process->name = name;

    process->threads = ARRAY_INIT(sizeof(struct thread *));
    process->pid = 0;

    sched_process_arch_info_init(process);
    sched_process_algo_info_init(process);
}