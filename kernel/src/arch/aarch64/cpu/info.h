/*
 * kernel/src/arch/aarch64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

#include "sched/info.h"
#include "sched/thread.h"

struct process;
struct cpu_info {
    struct process *process;

    struct list pagemap_node;
    struct list cpu_list;

    uint64_t spur_intr_count;

    uint32_t interface_number;
    uint32_t processor_number;

    uint64_t affinity;
    struct thread *idle_thread;

    uint16_t spe_overflow_interrupt;
    uint16_t icid;

    void *gic_its_pend_page;
    void *gic_its_prop_page;

    bool is_active : 1;
    bool in_lpi : 1;

    struct sched_percpu_info info;
};

extern struct list g_cpu_list;
struct cpu_info *cpu_mut_for_intr_number(uint32_t intr_number);