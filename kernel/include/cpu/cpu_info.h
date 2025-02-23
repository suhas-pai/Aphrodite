/*
 * kernel/include/cpu/info.h
 * © suhas pai
 */

#pragma once

#include <lib/list.h>
#include "sched/info.h"

#include "limine.h"

struct cpu_info;
struct cpu_info_base {
    struct list alarm_list;
    struct list cpu_list;
    struct list pagemap_node;

    struct thread *idle_thread;

    // Keep track of spurious interrupts for every cpu.
    uint64_t spur_intr_count;
    struct sched_percpu_info sched_info;

    void *percpu_base;
};

#define CPU_INFO_BASE_INIT(name) \
    .alarm_list = LIST_INIT(name.alarm_list), \
    .cpu_list = LIST_INIT(name.cpu_list), \
    .pagemap_node = LIST_INIT(name.pagemap_node), \
    .idle_thread = nullptr, \
    .spur_intr_count = 0, \
    .sched_info = SCHED_PERCPU_INFO_INIT(name.sched_info)

#define __percpu __attribute__((used, section(".percpu")))

extern char percpu_start[];
extern char percpu_end[];

#define percpu(var) ({\
    const auto h_var(cpu) = this_cpu(); \
    percpu_of(h_var(cpu), var); \
})

#define percpu_of(cpu, var) ({\
    assert_msg(cpu->percpu_base != NULL, "percpu: base is NULL");\
    assert_msg((uintptr_t)&(var) >= (uintptr_t)percpu_start && \
               (uintptr_t)&(var) < (uintptr_t)percpu_end, \
               "percpu: accessing " #var " is out of bounds");\
    (typeof(var) *)\
        ((uintptr_t)cpu->percpu_base + \
         ((uintptr_t)&(var) - (uintptr_t)percpu_start)); \
})

void cpu_info_base_init(struct cpu_info *cpu);

extern struct cpu_info g_base_cpu_info;
const struct cpu_info *this_cpu();

struct list *cpus_get_list();

struct cpu_info *this_cpu_mut();
struct cpu_info *cpu_add(const struct limine_mp_info *info);

typedef uint32_t cpu_id_t;

bool cpu_in_bad_state();
cpu_id_t cpu_get_id(const struct cpu_info *cpu);

const struct cpu_info *cpu_for_id(cpu_id_t id);
struct cpu_info *cpu_for_id_mut(cpu_id_t id);
