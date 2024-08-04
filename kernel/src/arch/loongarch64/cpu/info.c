/*
 * kernel/src/arch/loongarch64/cpu/info.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "cpu/util.h"
#include "lib/list.h"

#include "sched/process.h"
#include "sched/thread.h"

#include "info.h"

static struct thread idle_thread;
__hidden struct cpu_info g_base_cpu_info = {
    .cpu_list = LIST_INIT(g_base_cpu_info.cpu_list),
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),

    .process = &kernel_process,
    .idle_thread = &idle_thread,

    .in_exception = false,
};

struct list g_cpu_list = LIST_INIT(g_cpu_list);
static bool g_base_cpu_init = false;

__optimize(3) const struct cpu_info *base_cpu() {
    assert(g_base_cpu_init);
    return &g_base_cpu_info;
}

extern void sched_set_current_thread(struct thread *thread);

void cpu_early_init() {
    sched_thread_init(&idle_thread, &kernel_process, cpu_idle);
    sched_set_current_thread(&kernel_main_thread);
}

void cpu_init() {
    g_base_cpu_info.core_id = csr_read(cpuid);
    list_add(&g_cpu_list, &g_base_cpu_info.cpu_list);
}

__optimize(3)
struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    (void)info;
    verify_not_reached();
}

__optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}
