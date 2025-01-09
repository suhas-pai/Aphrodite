/*
 * kernel/include/cpu/smp.h
 * © suhas pai
 */

#pragma once

struct smp_boot_info {
    struct cpu_info *cpu;
    _Atomic bool booted;
};

#define SMP_BOOT_INFO_INIT(cpu_) \
    ((struct smp_boot_info){ \
        .cpu = (cpu_), \
        .booted = false, \
    })

void smp_init();
void smp_boot_all_cpus();
