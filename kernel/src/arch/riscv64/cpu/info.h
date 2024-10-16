/*
 * kernel/src/arch/riscv64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

#include "lib/list.h"
#include "lib/time.h"

struct process;
struct cpu_info {
    struct cpu_info_base;

    uint16_t hart_id;
    uint16_t cbo_size;
    uint16_t cmo_size;

    usec_t timer_start;
    bool in_exception : 1;

    uint8_t isr_oode;
    uint64_t imsic_phys;

    volatile uint32_t *imsic_page;
};

struct cpus_info {
    uint64_t timebase_frequency;
};

const struct cpus_info *get_cpus_info();
