/*
 * kernel/include/arch/aarch64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

struct process;
struct cpu_info {
    struct cpu_info_base;

    uint64_t mpidr;
    uint64_t affinity;

    uint16_t spe_overflow_interrupt;
    uint16_t icid;
    uint32_t processor_id;

    // In EL1 (The Kernel), we use a separate stack, that we store in SP_EL1.
    // By default, we use SP_EL0, because SPSEL=0. In an interrupt/exception,
    // SPSEL is set to 1, so SP_EL1 is used.

    struct page *irq_stack;

    bool in_lpi : 1;
    bool in_exception : 1;
    bool called_eoi : 1;
};
