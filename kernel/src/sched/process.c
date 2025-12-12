/*
 * kernel/src/sched/process.c
 * © suhas pai
 */

#include "sched/process.h"

__hidden struct process kernel_process = {
    .pagemap = (struct pagemap){
    #if PGT_HAS_SPLIT_ROOT
        .lower_root = nullptr, // setup later
        .higher_root = nullptr, // setup later
    #else
        .root = nullptr, // setup later
    #endif /* PGT_HAS_SPLIT_ROOT */

        .cpu_list = LIST_INIT(kernel_process.pagemap.cpu_list),
        .cpu_lock = SPINLOCK_INIT(),
        .addrspace = ADDRSPACE_INIT(kernel_process.pagemap.addrspace),
        .addrspace_lock = SPINLOCK_INIT(),
        .refcount = REFCOUNT_CREATE_MAX(),
    },

    .threads = ARRAY_INIT(sizeof(struct thread *)),
    .name = STRING_STATIC("kernel"),

    .pid = 0
};

void
sched_process_init(struct process *const process, const struct string name) {
    process->pagemap = pagemap_empty();
    process->name = name;

    process->threads = ARRAY_INIT(sizeof(struct thread *));
    process->pid = 0;

    sched_process_arch_info_init(process);
    sched_process_algo_info_init(process);
}