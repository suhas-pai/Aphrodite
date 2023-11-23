/*
 * kernel/src/sched/process.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "mm/pagemap.h"

struct process {
    struct pagemap *pagemap;
    struct array threads;

    char *name;
};

extern struct process kernel_process;