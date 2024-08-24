/*
 * kernel/src/cpu/info.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

#include "lib/list.h"
#include "sched/info.h"

#include "limine.h"

struct cpu_info;
struct cpu_info_base {
    struct process *process;

    struct list alarm_list;
    struct list cpu_list;
    struct list pagemap_node;

    struct thread *idle_thread;

    // Keep track of spurious interrupts for every cpu.
    uint64_t spur_intr_count;
    struct sched_percpu_info sched_info;
};

#define CPU_INFO_BASE_INIT(name, process_) \
    .process = (process_), \
    .pagemap_node = LIST_INIT(name.pagemap_node), \
    .cpu_list = LIST_INIT(name.cpu_list), \
    .alarm_list = LIST_INIT(name.alarm_list), \
    .idle_thread = NULL, \
    .spur_intr_count = 0, \
    .sched_info = SCHED_PERCPU_INFO_INIT()

void cpu_info_base_init(struct cpu_info *cpu, struct process *process);

extern struct cpu_info g_base_cpu_info;
const struct cpu_info *this_cpu();

struct list *cpus_get_list();

struct cpu_info *this_cpu_mut();
struct cpu_info *cpu_add(const struct limine_smp_info *info);

bool cpu_in_bad_state();

typedef uint32_t cpu_id_t;
cpu_id_t cpu_get_id(const struct cpu_info *cpu);

const struct cpu_info *cpu_for_id(cpu_id_t id);
struct cpu_info *cpu_for_id_mut(cpu_id_t id);
