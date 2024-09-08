/*
 * kernel/src/cpu/smp.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"
#include "dev/printk.h"

#include "sched/scheduler.h"
#include "sys/boot.h"

#include "smp.h"

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
    #error "Unkonwn architecture"
#endif

#define bsp_field VAR_CONCAT(bsp_, field)

void smp_init() {
    const struct limine_smp_response *const smp_resp = boot_get_smp();
    if (smp_resp == NULL) {
        return;
    }

#if !defined(__loongarch64)
    struct limine_smp_info *const *const cpu_list = smp_resp->cpus;
    const uint64_t cpu_count = smp_resp->cpu_count;

    for (uint64_t i = 0; i != cpu_count; i++) {
        if (cpu_list[i]->field != smp_resp->bsp_field) {
            cpu_add(cpu_list[i]);
        }
    }
#endif /* !defined(__loongarch64) */
}

void smp_boot_all_cpus() {
#if defined(__x86_64__) || defined(__aarch64__)
    const struct limine_smp_response *const smp_resp = boot_get_smp();
    if (smp_resp == NULL) {
        return;
    }

    struct limine_smp_info *const *const cpu_list = smp_resp->cpus;
    const uint64_t cpu_count = smp_resp->cpu_count;

    if (cpu_count == 1) {
        return;
    }

    printk(LOGLEVEL_INFO, "smp: booting all cpus\n");
    with_irqs_disabled({
        for (uint64_t i = 0; i != cpu_count; i++) {
            if (cpu_list[i]->field == smp_resp->bsp_field) {
                continue;
            }

            struct cpu_info *const cpu =
                cpu_for_id_mut(cpu_list[i]->processor_id);

            struct smp_boot_info boot_info = SMP_BOOT_INFO_INIT(cpu);
            assert_msg(cpu != NULL,
                       "smp: failed to find cpu-info for "
                       "processor-id %" PRIu32 "\n",
                       cpu->processor_id);

            sched_init_on_cpu(cpu);

            cpu_list[i]->extra_argument = (uint64_t)&boot_info;
            cpu_list[i]->goto_address = arch_init_for_smp;

            while (!atomic_load_explicit(&boot_info.booted,
                                         memory_order_seq_cst))
            {
                cpu_pause();
            }
        }
    });
#endif /* defined(__x86_64__) || defined(__aarch64__) */
}