/*
 * kernel/src/cpu/smp.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cpu/smp.h"
#include "dev/printk.h"

#include "sched/scheduler.h"
#include "sys/boot.h"

extern void arch_init_for_smp();

#if defined(__x86_64__)
    #define field lapic_id
#elif defined(__aarch64__)
    #define field mpidr
#elif defined(__riscv64)
    #define field hartid
#elif defined(__loongarch64)
    #define field
#else
    #error "Unknown architecture"
#endif

#define bsp_field VAR_CONCAT(bsp_, field)

void smp_init() {
    const struct limine_mp_response *const smp_resp = boot_get_mp();
    if (smp_resp == nullptr) {
        return;
    }

#if !defined(__loongarch64)
    struct limine_mp_info *const *const cpu_list = smp_resp->cpus;
    const uint64_t cpu_count = smp_resp->cpu_count;

    for (uint64_t i = 0; i != cpu_count; i++) {
    #ifdef __aarch64__
        const uint64_t processor_id = cpu_list[i]->mpidr;
    #else
        const uint64_t processor_id = cpu_list[i]->processor_id;
    #endif

        printk(LOGLEVEL_INFO,
               "smp: found cpu with processor-id %" PRIu64 "\n",
               processor_id);

        if (cpu_list[i]->field != smp_resp->bsp_field) {
            cpu_add(cpu_list[i]);
        }
    }
#endif /* !defined(__loongarch64) */
}

void smp_boot_all_cpus() {
#if defined(__x86_64__) || defined(__aarch64__)
    const struct limine_mp_response *const mp_resp = boot_get_mp();
    if (mp_resp == nullptr) {
        return;
    }

    struct limine_mp_info *const *const info_list = mp_resp->cpus;
    const uint64_t cpu_count = mp_resp->cpu_count;

    if (cpu_count == 1) {
        return;
    }

    printk(LOGLEVEL_INFO, "smp: booting all %" PRIu64 " cpus\n", cpu_count);
    for (uint64_t i = 0; i != cpu_count; i++) {
        struct limine_mp_info *const info = info_list[i];
        if (info->field == mp_resp->bsp_field) {
            continue;
        }

    #ifdef __aarch64__
        const uint64_t processor_id = info->mpidr;
    #else
        const uint64_t processor_id = info->processor_id;
    #endif

        struct cpu_info *const cpu = cpu_for_id_mut(processor_id);
        struct smp_boot_info boot_info = SMP_BOOT_INFO_INIT(cpu);

        assert_msg(cpu != nullptr,
                   "smp: failed to find cpu-info for "
                   "processor-id %" PRIu64 "\n",
                   processor_id);

        sched_init_on_cpu(cpu);
        with_intr_disabled({
            info->extra_argument = (uint64_t)&boot_info;
            atomic_store_explicit(
                (_Atomic uint64_t *)&info->goto_address,
                (uint64_t)arch_init_for_smp,
                memory_order_seq_cst);

            while (!atomic_load_explicit(&boot_info.booted,
                                         memory_order_seq_cst))
            {
                cpu_pause();
            }
        });
    }
#endif /* defined(__x86_64__) || defined(__aarch64__) */

    printk(LOGLEVEL_INFO, "smp: finished booting all cpus\n");
}