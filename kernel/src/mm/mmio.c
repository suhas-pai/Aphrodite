/*
 * kernel/src/mm/mmio.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "sched/process.h"

#include "kmalloc.h"
#include "mmio.h"
#include "pgmap.h"

static struct address_space mmio_space = ADDRSPACE_INIT(mmio_space);
static struct spinlock g_mmio_space_lock = SPINLOCK_INIT();

enum prot_fail {
    PROT_FAIL_NONE,
    PROT_FAIL_PROT_NONE,
    PROT_FAIL_PROT_EXEC,
    PROT_FAIL_PROT_USER
};

__debug_optimize(3)
static inline enum prot_fail verify_prot(const prot_t prot) {
    if (__builtin_expect(prot == PROT_NONE, 0)) {
        return PROT_FAIL_PROT_NONE;
    }

    if (__builtin_expect(prot & PROT_EXEC, 0)) {
        return PROT_FAIL_PROT_EXEC;
    }

    if (__builtin_expect(prot & PROT_USER, 0)) {
        return PROT_FAIL_PROT_USER;
    }

    return PROT_FAIL_NONE;
}

__debug_optimize(3) static uint64_t
find_virt_addr(const struct range phys_range,
               const struct range in_range,
               struct mmio_region *const mmio)
{
    for (int8_t i = PGT_LEVEL_COUNT - 1; i > 1; i--) {
        const struct largepage_level_info *const level_info =
            &largepage_level_info_list[i];

        if (!level_info->is_supported || phys_range.size < level_info->size) {
            continue;
        }

        if (!has_align(phys_range.front, level_info->size)) {
            continue;
        }

        const uint64_t virt_addr =
            addrspace_find_space_and_add_node(&mmio_space,
                                              in_range,
                                              &mmio->node,
                                              level_info->order);

        if (virt_addr != ADDRSPACE_INVALID_ADDR) {
            return virt_addr;
        }
    }

    return
        addrspace_find_space_and_add_node(&mmio_space,
                                          in_range,
                                          &mmio->node,
                                          /*pagesize_order=*/0);
}

// (at least) 16kib of guard space
#define GUARD_PAGE_SIZE max(kib(16), PAGE_SIZE)

static struct mmio_region *
map_mmio_region(const struct range phys_range,
                const prot_t prot,
                const uint64_t flags)
{
    const struct range in_range =
        range_create_end(VMAP_BASE + GUARD_PAGE_SIZE, VMAP_END);

    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));
    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to allocate mmio_region to map phys-range "
               RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    mmio->node = ADDRSPACE_NODE_INIT(mmio->node, &mmio_space);
    mmio->node.range.size = phys_range.size + GUARD_PAGE_SIZE;

    const int flag = spin_acquire_save_irq(&g_mmio_space_lock);
    const uint64_t virt_addr = find_virt_addr(phys_range, in_range, mmio);

    if (virt_addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_restore_irq(&g_mmio_space_lock, flag);
        kfree(mmio);

        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to find a suitable virtual-address range "
               "to map phys-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));

        return NULL;
    }

    struct range virt_range = RANGE_EMPTY();
    if (!range_create_and_verify(virt_addr, phys_range.size, &virt_range)) {
        spin_release_restore_irq(&g_mmio_space_lock, flag);
        kfree(mmio);

        printk(LOGLEVEL_WARN,
               "vmap_mmio(): virtual-address range to map "
               "phys-range " RANGE_FMT " overflows\n",
               RANGE_FMT_ARGS(phys_range));

        return NULL;
    }

    const bool map_success =
        arch_make_mapping(&kernel_process.pagemap,
                          phys_range,
                          virt_addr,
                          prot,
                          flags & __VMAP_MMIO_WT ?
                            VMA_CACHEKIND_WRITETHROUGH : VMA_CACHEKIND_MMIO,
                          /*is_overwrite=*/false);

    spin_release_restore_irq(&g_mmio_space_lock, flag);
    if (!map_success) {
        kfree(mmio);
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to map phys-range " RANGE_FMT " to virtual "
               "range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range),
               RANGE_FMT_ARGS(virt_range));

        return NULL;
    }

    mmio->base = (volatile void *)virt_addr;
    mmio->size = phys_range.size;

    return mmio;
}

struct mmio_region *
vmap_mmio(const struct range phys_range,
          const prot_t prot,
          const uint64_t flags)
{
    if (__builtin_expect(range_empty(phys_range), 0)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): attempting to map empty phys-range\n");
        return NULL;
    }

    if (__builtin_expect(!range_has_align(phys_range, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): phys-range " RANGE_FMT " isn't aligned to the "
               "page size\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    switch (verify_prot(prot)) {
        case PROT_FAIL_NONE:
            break;
        case PROT_FAIL_PROT_NONE:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "w/o access permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
        case PROT_FAIL_PROT_EXEC:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "with execute permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
        case PROT_FAIL_PROT_USER:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "with user permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
    }

    return map_mmio_region(phys_range, prot, flags);
}

bool vunmap_mmio(struct mmio_region *const region) {
    const struct pgunmap_options options = {
        .free_pages = false,
        .dont_split_large_pages = true
    };

    const struct range virt_range = mmio_region_get_range(region);

    const int flag = spin_acquire_save_irq(&g_mmio_space_lock);
    const bool result =
        pgunmap_at(&kernel_process.pagemap,
                   virt_range,
                   /*map_options=*/NULL,
                   &options);

    if (!result) {
        spin_release_restore_irq(&g_mmio_space_lock, flag);
        printk(LOGLEVEL_WARN,
               "mm: failed to unmap mmio region at " RANGE_FMT "\n",
               RANGE_FMT_ARGS(virt_range));

        return false;
    }

    addrspace_remove_node(&region->node);
    spin_release_restore_irq(&g_mmio_space_lock, flag);
    kfree(region);

    return true;
}

__debug_optimize(3)
struct range mmio_region_get_range(const struct mmio_region *const region) {
    return RANGE_INIT((uint64_t)region->base, region->size);
}