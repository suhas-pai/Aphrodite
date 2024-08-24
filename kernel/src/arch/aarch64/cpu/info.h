/*
 * kernel/src/arch/aarch64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

struct process;
struct cpu_info {
    struct cpu_info_base;
    uint64_t mpidr;

    uint32_t processor_id;
    uint64_t affinity;

    uint16_t spe_overflow_interrupt;
    uint16_t icid;

    void *gic_its_pend_page;
    void *gic_its_prop_page;

    bool is_active : 1;
    bool in_lpi : 1;
    bool in_exception : 1;
};
