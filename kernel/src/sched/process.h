/*
 * kernel/src/sched/process.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "lib/adt/string.h"

#include "mm/pagemap.h"
#include "sched/arch.h"

#include "info.h"

struct process {
    struct pagemap *pagemap;
    struct array threads;

    struct string name;
    struct sched_process_info sched_info;
    struct process_arch_info arch_info;

    struct list stack_page_list;
    int pid;
};

extern struct process kernel_process;

void
sched_process_init(struct process *process,
                   struct pagemap *pagemap,
                   struct string name);
