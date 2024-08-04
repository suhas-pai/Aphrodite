/*
 * kernel/src/sched/process.c
 * © suhas pai
 */

#include "process.h"

__hidden struct process kernel_process = {
    .pagemap = (struct pagemap){
    #if PAGEMAP_HAS_SPLIT_ROOT
        .lower_root = NULL, // setup later
        .higher_root = NULL, // setup later
    #else
        .root = NULL, // setup later
    #endif /* defined(__aarch64__)*/

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