/*
 * kernel/src/arch/riscv64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

#include "lib/list.h"
#include "mm/pagemap.h"

struct pagemap;
struct cpu_info {
    struct pagemap *pagemap;
    struct list pagemap_node;

    struct thread *idle_thread;
    uint64_t spur_int_count;

    uint16_t cbo_size;
    uint16_t cmo_size;

    bool is_active : 1;
};