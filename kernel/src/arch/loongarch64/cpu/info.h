/*
 * kernel/src/arch/loongarch64/cpu/info.h
 * © suha spai
 */

#pragma once
#include <stdbool.h>

#include "cpu/cpu_info.h"
#include "lib/list.h"

struct cpu_info {
    struct list cpu_list;
    struct list pagemap_node;

    struct process *process;
    struct thread *idle_thread;

    uint8_t core_id;
    bool in_exception : 1;
};
