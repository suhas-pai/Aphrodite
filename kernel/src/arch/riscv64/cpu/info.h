/*
 * kernel/src/arch/riscv64/cpu/info.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "mm/pagemap.h"

struct pagemap;
struct cpu_info {
    struct pagemap *pagemap;
    struct list pagemap_node;

    uint64_t spur_int_count;

    uint16_t cbo_size;
    uint16_t cmo_size;
};

const struct cpu_info *get_base_cpu_info();
const struct cpu_info *get_cpu_info();
struct cpu_info *get_cpu_info_mut();