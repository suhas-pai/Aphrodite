/*
 * kernel/src/sched/process.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "lib/adt/string_view.h"

#include "mm/pagemap.h"
#include "info.h"

struct process {
    struct pagemap *pagemap;
    struct array threads;

    struct string_view name;
    struct sched_process_info sched_info;

    int pid;
};

extern struct process kernel_process;
