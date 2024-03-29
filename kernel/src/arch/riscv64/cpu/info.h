/*
 * kernel/src/arch/riscv64/cpu/info.h
 * © suhas pai
 */

#pragma once

#include "cpu/cpu_info.h"
#include "lib/list.h"

struct process;
struct cpu_info {
    struct process *process;
    struct list pagemap_node;

    struct thread *idle_thread;

    uint64_t spur_intr_count;
    uint16_t hart_id;

    uint16_t cbo_size;
    uint16_t cmo_size;

    bool is_active : 1;
};

struct cpus_info {
    uint64_t timebase_frequency;
};

const struct cpus_info *get_cpus_info();