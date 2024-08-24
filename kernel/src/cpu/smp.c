/*
 * kernel/src/cpu/smp.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "dev/printk.h"
#include "mm/kmalloc.h"

#include "sched/thread.h"
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
#if defined(__x86_64__)
    printk(LOGLEVEL_INFO, "smp: booting all cpus\n");

    const struct limine_smp_response *const smp_resp = boot_get_smp();
    if (smp_resp == NULL) {
        return;
    }

    struct limine_smp_info *const *const cpu_list = smp_resp->cpus;
    const uint64_t cpu_count = smp_resp->cpu_count;

    const int flag = disable_irqs_if_enabled();
    for (uint64_t i = 0; i != cpu_count; i++) {
        struct thread *const fake_thread = kmalloc(sizeof(*fake_thread));

        assert_msg(fake_thread != NULL, "smp: failed to alloc sched-thread");
        sched_thread_init(fake_thread,
                          &kernel_process,
                          NULL, // Fixed in arch_init_for_smp()
                          arch_init_for_smp);

        cpu_list[i]->extra_argument = (uint64_t)fake_thread;
        cpu_list[i]->goto_address = arch_init_for_smp;
    }

    enable_irqs_if_flag(flag);
#endif /* defined(__x86_64__) */
}