/*
 * kernel/src/cpu/smp.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "lib/macros.h"
#include "sys/boot.h"

void smp_init() {
    const struct limine_smp_response *const smp_resp = boot_get_smp();
    if (smp_resp == NULL) {
        return;
    }

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