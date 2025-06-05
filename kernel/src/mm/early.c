/*
 * kernel/src/mm/early.c
 * © suhas pai
 */

#include <lib/align.h>
#include <lib/size.h>

#include "dev/printk.h"

#include "mm/early.h"
#include "mm/kmalloc.h"
#include "mm/memmap.h"
#include "mm/physalloc.h"
#include "mm/walker.h"
#include "mm/zone.h"

#include "sched/process.h"
#include "sys/boot.h"

struct freepage_array_info {
    struct list list;
    struct list asc_list;

    struct mm_memmap *memmap;

    // Number of available pages in this freepage_array_info struct.
    uint64_t avail_page_count;

    // Count of contiguous pages, starting from this one, including the count of
    // already allocated pages.
    uint64_t total_page_count;
} __page_aligned;

static inline void
freepage_array_info_init(struct freepage_array_info *const info,
                        const uint64_t page_count)
{
    list_init(&info->list);
    list_init(&info->asc_list);

    info->avail_page_count = page_count;
    info->total_page_count = page_count;
}

_Static_assert(sizeof(struct freepage_array_info) <= PAGE_SIZE,
               "freepage_array_info struct must be small enough to store on a "
               "single page");

// Use two lists of usable pages:
//  One that stores pages in ascending address order,
//  One that stores pages in ascending order of the number of free pages.

// The address ascending order is needed later in post-arch setup to mark all
// system-crucial pages, while the ascending free-page list is used so smaller
// areas are emptied before larger areas are used.

static struct list g_freepage_list = LIST_INIT(g_freepage_list);
static struct list g_asc_freelist = LIST_INIT(g_asc_freelist);

static uint64_t g_total_free_pages = 0;
static uint64_t g_total_free_pages_remaining = 0;

__debug_optimize(3)
static void add_to_asc_list(struct freepage_array_info *const info) {
    struct freepage_array_info *iter = nullptr;
    struct freepage_array_info *prev =
        parent_of(&g_asc_freelist, struct freepage_array_info, asc_list);

    list_foreach(&g_asc_freelist, asc_list, iter) {
        if (info->avail_page_count < iter->avail_page_count) {
            break;
        }

        prev = iter;
    }

    list_add(&prev->asc_list, &info->asc_list);
}

__debug_optimize(3)
static void claim_pages(const struct mm_memmap *const memmap) {
#if defined(__aarch64__) && defined(AARCH64_CONFIG_16K_PAGES)
    struct range phys_range = memmap->range;
    if (!range_align_in(phys_range, PAGE_SIZE, &phys_range)) {
        printk(LOGLEVEL_WARN,
               "early; failed to claim memmap at phys-range " RANGE_FMT ", "
               "couldn't align to 16kib page size\n",
               RANGE_FMT_ARGS(phys_range));
        return;
    }
#else
    const struct range phys_range = memmap->range;
#endif /* defined(__aarch64__) && defined(AARCH64_CONFIG_16K_PAGES) */

    struct freepage_array_info *const info = phys_to_virt(phys_range.front);
    const uint64_t page_count = PAGE_COUNT(memmap->range.size);

    freepage_array_info_init(info, page_count);

    g_total_free_pages += page_count;
    g_total_free_pages_remaining += page_count;

    struct freepage_array_info *prev =
        list_tail(&g_freepage_list, struct freepage_array_info, list);

    if (&prev->list != &g_freepage_list) {
        /*
         * g_freepage_list must be in the ascending order of the physical
         * address. Try finding the appropriate place in the linked-list if
         * we're not directly after the tail memmap.
         */

        if (prev > info) {
            prev =
                parent_of(&g_freepage_list, struct freepage_array_info, list);

            struct freepage_array_info *iter = nullptr;
            list_foreach(&g_freepage_list, list, iter) {
                if (info < iter) {
                    break;
                }

                prev = iter;
            }

            // Merge with the memmap after prev if possible.

            struct freepage_array_info *next = list_next(prev, list);
            const uint64_t info_size = info->avail_page_count << PAGE_SHIFT;

            if ((void *)info + info_size == (void *)next) {
                info->avail_page_count += page_count;
                info->total_page_count += page_count;

                list_deinit(&next->list);
            }
        }

        // Merge with the previous memmap, if possible.
        if (&prev->list != &g_freepage_list) {
            const uint64_t back_size = prev->avail_page_count << PAGE_SHIFT;
            if ((void *)prev + back_size == (void *)info) {
                prev->avail_page_count += page_count;
                prev->total_page_count += page_count;

                struct freepage_array_info *const prev_next =
                    list_next_safe(prev, list, &g_freepage_list);

                if (prev_next != nullptr &&
                    prev->avail_page_count > prev_next->avail_page_count)
                {
                    list_remove(&prev->asc_list);
                    add_to_asc_list(prev);
                }

                return;
            }
        }
    }

    list_add(&prev->list, &info->list);
    add_to_asc_list(info);
}

__debug_optimize(3) uint64_t early_alloc_page() {
    if (__builtin_expect(list_empty(&g_asc_freelist), 0)) {
        printk(LOGLEVEL_ERROR, "mm: ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepage_array_info *const info =
        list_head(&g_asc_freelist, struct freepage_array_info, asc_list);

    info->avail_page_count -= 1;
    if (info->avail_page_count == 0) {
        list_deinit(&info->asc_list);
        list_deinit(&info->list);
    }

    // Take the last page out of the list, because the first page stores the
    // freepage_array_info struct.

    const uint64_t free_page =
        virt_to_phys(info) + (info->avail_page_count << PAGE_SHIFT);

    zero_page(phys_to_virt(free_page));
    g_total_free_pages_remaining--;

    return free_page;
}

#if defined(DEBUG_EARLY_ALLOC)
#define print_freepage_list(head, field, name) \
    do { \
        struct freepage_array_info *iter = nullptr;  \
        uint32_t i = 1; \
        \
        printk(LOGLEVEL_INFO, "mm: printing list " name "\n"); \
        list_foreach(head, field, iter) { \
            printk(LOGLEVEL_INFO, \
                   "\tmm: #%u: freepage_array_info at " RANGE_FMT ", " \
                   "avail_page_count=%" PRIu64 \
                   ", total_page_count=%" PRIu64 "\n", \
                   i, \
                   RANGE_FMT_ARGS( \
                    RANGE_INIT(virt_to_phys(iter), \
                               iter->total_page_count << PAGE_SHIFT)), \
                   iter->avail_page_count, \
                   iter->total_page_count); \
 \
            i++; \
        } \
    } while (false)
#endif /* defined(DEBUG_EARLY_ALLOC) */

__debug_optimize(3) uint64_t early_alloc_large_page(const pg_level_t level) {
    if (__builtin_expect(list_empty(&g_asc_freelist), 0)) {
        printk(LOGLEVEL_ERROR, "mm: ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepage_array_info *info = nullptr;

    uint64_t free_page = INVALID_PHYS;
    bool is_in_middle = false;

    const uint64_t alloc_amount =
        1ull << lg_page_level_info_list[level - 1].order;

    list_foreach(&g_asc_freelist, asc_list, info) {
        const uint64_t avail_page_count = info->avail_page_count;
        if (avail_page_count < alloc_amount) {
            continue;
        }

        uint64_t info_phys = virt_to_phys(info);
        uint64_t page_phys =
            info_phys + ((avail_page_count - alloc_amount) << PAGE_SHIFT);

        const uint64_t boundary = alloc_amount << PAGE_SHIFT;
        if (!has_align(page_phys, boundary)) {
            page_phys = align_down(page_phys, boundary);
            if (page_phys < info_phys) {
                continue;
            }

            is_in_middle = true;
        }

        free_page = page_phys;
        break;
    }

    if (free_page == INVALID_PHYS) {
        return INVALID_PHYS;
    }

    if (is_in_middle) {
        struct freepage_array_info *prev = info;
        const uint64_t old_info_count = info->avail_page_count;

        info->avail_page_count = PAGE_COUNT(free_page - virt_to_phys(info));
        info->total_page_count = info->avail_page_count + alloc_amount;

        if (info->avail_page_count != 0) {
            struct freepage_array_info *const info_prev =
                list_prev_safe(info, asc_list, &g_asc_freelist);

            if (info_prev != nullptr) {
                if (info_prev->avail_page_count > info->avail_page_count) {
                    list_remove(&info->asc_list);
                    add_to_asc_list(info);
                }
            }
        } else {
            prev = list_prev(info, list);

            list_deinit(&info->asc_list);
            list_deinit(&info->list);
        }

        const uint64_t new_info_phys = free_page + (alloc_amount << PAGE_SHIFT);
        const uint64_t new_info_count =
            old_info_count - info->avail_page_count - alloc_amount;

        if (new_info_count != 0) {
            struct freepage_array_info *const new_info =
                phys_to_virt(new_info_phys);

            freepage_array_info_init(new_info, /*page_count=*/new_info_count);

            list_add(&prev->list, &new_info->list);
            add_to_asc_list(new_info);
        }
    } else {
        info->avail_page_count -= alloc_amount;
        if (info->avail_page_count != 0) {
            const struct freepage_array_info *const prev =
                list_prev_safe(info, asc_list, &g_asc_freelist);

            if (prev != nullptr) {
                if (prev->avail_page_count > info->avail_page_count) {
                    list_remove(&info->asc_list);
                    add_to_asc_list(info);
                }
            }
        } else {
            list_deinit(&info->list);
            list_deinit(&info->asc_list);
        }
    }

#if defined(DEBUG_EARLY_ALLOC)
    print_freepage_list(&g_freepage_list, list, "freepage-list");
    print_freepage_list(&g_asc_freelist, asc_list, "asc-freepage-list");
#endif /* defined(DEBUG_EARLY_ALLOC) */

    zero_multiple_pages(phys_to_virt(free_page), alloc_amount);
    g_total_free_pages_remaining -= alloc_amount;

    return free_page;
}

__debug_optimize(3) uint64_t mm_get_total_page_count() {
    return g_total_free_pages;
}

__hidden uint64_t KERNEL_BASE = 0;

__debug_optimize(3) void mm_early_init() {
    mm_for_each_memmap(memmap) {
        switch (memmap->kind) {
            case MM_MEMMAP_KIND_NONE:
                verify_not_reached();
            case MM_MEMMAP_KIND_USABLE:
                claim_pages(memmap);
                break;
            case MM_MEMMAP_KIND_RESERVED:
            case MM_MEMMAP_KIND_ACPI_RECLAIMABLE:
            case MM_MEMMAP_KIND_ACPI_NVS:
            case MM_MEMMAP_KIND_BAD_MEMORY:
                break;
            case MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE:
                // Because we're claiming this kind of memmap later, they are
                // still represented in the structpage table.

                // Avoid adding these pages for now. We have to switch to using
                // our own stack before these pages can be used.

                //structpage_page_count += PAGE_COUNT(memmap->range.size);
                break;
            case MM_MEMMAP_KIND_EXEC_AND_MODULES:
            case MM_MEMMAP_KIND_FRAMEBUFFER:
                break;
        }
    }
}

__debug_optimize(3) void mm_init() {
    printk(LOGLEVEL_INFO, "mm: hhdm at %p\n", (void *)HHDM_OFFSET);
    printk(LOGLEVEL_INFO, "mm: kernel at %p\n", (void *)KERNEL_BASE);

    uint32_t index = 0;
    mm_for_each_memmap(memmap) {
        const char *type_desc = "<unknown>";
        switch (memmap->kind) {
            case MM_MEMMAP_KIND_NONE:
                verify_not_reached();
            case MM_MEMMAP_KIND_USABLE:
                type_desc = "usable";
                break;
            case MM_MEMMAP_KIND_RESERVED:
                type_desc = "reserved";
                break;
            case MM_MEMMAP_KIND_ACPI_RECLAIMABLE:
                type_desc = "acpi-reclaimable";
                break;
            case MM_MEMMAP_KIND_ACPI_NVS:
                type_desc = "acpi-nvs";
                break;
            case MM_MEMMAP_KIND_BAD_MEMORY:
                type_desc = "bad-memory";
                break;
            case MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE:
                type_desc = "bootloader-reclaimable";
                break;
            case MM_MEMMAP_KIND_EXEC_AND_MODULES:
                type_desc = "kernel-and-modules";
                break;
            case MM_MEMMAP_KIND_FRAMEBUFFER:
                type_desc = "framebuffer";
                break;
        }

        printk(LOGLEVEL_INFO,
               "mm: memmap %" PRIu32 ": [" RANGE_FMT "] %s\n",
               index + 1,
               RANGE_FMT_ARGS(memmap->range),
               type_desc);

        index++;
    }

    printk(LOGLEVEL_INFO,
           "mm: system has %" PRIu64 " usable pages\n",
           g_total_free_pages);

    printk(LOGLEVEL_INFO,
           "mm: system has " SIZE_UNIT_FMT " of available memory\n",
           SIZE_UNIT_FMT_ARGS_ABBREV(g_total_free_pages * PAGE_SIZE));
}

__debug_optimize(3)
static inline void init_table_page(struct page *const page) {
    list_init(&page->table.delayed_free_list);
    refcount_init(&page->table.refcount);
}

__debug_optimize(3) void
mm_early_refcount_alloced_map(const uint64_t virt_addr, const uint64_t length) {
#if PAGEMAP_HAS_SPLIT_ROOT
    init_table_page(virt_to_page(kernel_process.pagemap.lower_root));
    init_table_page(virt_to_page(kernel_process.pagemap.higher_root));
#else
    init_table_page(virt_to_page(kernel_process.pagemap.root));
#endif /* PAGEMAP_HAS_SPLIT_ROOT */

    struct pg_walker walker;
    pgwalker_create(&walker,
                    virt_addr,
                    /*alloc_pgtable=*/nullptr,
                    /*free_pgtable=*/nullptr);

    for (pg_level_t level = (uint8_t)walker.level;
         level <= walker.top_level;
         level++)
    {
        struct page *const page = virt_to_page(walker.tables[level - 1]);
        if (page->table.refcount.count == 0) {
            init_table_page(page);
        }
    }

    // Track changes to the `walker.tables` array by
    //   (1) Seeing if the index of the lowest level ever reaches
    //       (PGT_PTE_COUNT - 1).
    //   (2) Comparing the previous level with the latest level. When the
    //       previous level is greater, the walker has incremented past a
    //       large page, so there may be some tables in between the large page
    //       level and the current level that need to be initialized.

    uint8_t prev_level = walker.level;
    bool prev_was_at_end =
        walker.indices[prev_level - 1] == PGT_PTE_COUNT(prev_level) - 1;

    const struct pgwalker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = nullptr,
        .free_pgtable_cb_info = nullptr,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = false,
    };

    enum pgwalker_result advance_result =
        pgwalker_next_with_options(&walker, walker.level, &iterate_options);

    if (__builtin_expect(advance_result != E_PGWALKER_OK, 0)) {
    fail:
        panic("mm: failed to setup kernel pagemap, result=%d\n",
              advance_result);
    }

    struct page *page = virt_to_page(walker.tables[walker.level - 1]);
    for (uint64_t i = 0;;) {
        if (__builtin_expect(prev_was_at_end, 0)) {
            // Here, we were previously at the end of a table, and have
            // incremented to a new table, possibly at a different level.

            page = virt_to_page(walker.tables[walker.level - 1]);
            init_table_page(page);

            // From our current level, and to some higher level, we have tables
            // whose pages have yet to be initialized. The highest level where
            // such a page exists is the highest level where the corresponding
            // index is also zero.

            for (pg_level_t level = (pg_level_t)walker.level + 1;
                 level <= walker.top_level;
                 level++)
            {
                pte_t *const table = walker.tables[level - 1];
                struct page *const table_page = virt_to_page(table);

                // On the page of the table just above the highest level table
                // with an uninitialized page, we just have to increment the
                // refcount and exit.

                if (walker.indices[level - 1] != 0) {
                    table_page->table.refcount.count++;
                    break;
                }

                init_table_page(table_page);
            }

            // Because we're refcounting only contiguous regions, we know that
            // `prev_was_at_end` must be false for the next iteration.

            prev_level = walker.level;
            prev_was_at_end = false;
        } else if (__builtin_expect(prev_level > walker.level, 0)) {
            // Here, we were at a higher level, pointing to a large page, before
            // going down to either a smaller large page or a normal sized leaf
            // page.

            // Initialize the tables in between our previous table, and our
            // current table.

            for (pg_level_t level = (pg_level_t)walker.level;
                 level < prev_level;
                 level++)
            {
                pte_t *const table = walker.tables[level - 1];
                struct page *const table_page = virt_to_page(table);

                init_table_page(table_page);
            }

            struct page *const prev_table_page =
                virt_to_page(walker.tables[prev_level - 1]);

            prev_table_page->table.refcount.count++;

            page = virt_to_page(walker.tables[walker.level - 1]);
            page->table.refcount.count++;

            // Because we're refcounting only contiguous regions, we know that
            // `prev_was_at_end` must be false for the next iteration, which is
            // also the present value/

            prev_level = walker.level;
        } else {
            page->table.refcount.count++;

            prev_level = walker.level;
            prev_was_at_end =
                walker.indices[prev_level - 1] == PGT_PTE_COUNT(prev_level) - 1;
        }

        i += PAGE_SIZE_AT_LEVEL(walker.level);
        if (i >= length) {
            break;
        }

        advance_result =
            pgwalker_next_with_options(&walker, walker.level, &iterate_options);

        if (__builtin_expect(advance_result != E_PGWALKER_OK, 0)) {
            goto fail;
        }
    }
}

static bool g_mapped_early_identity = false;
static pg_level_t g_mapped_early_top_level = 0;

static uint64_t g_mapped_early_phys = 0;
static uint64_t g_mapped_early_root_phys = 0;

__debug_optimize(3) void
mm_early_identity_map_phys(const uint64_t root_phys,
                           const uint64_t phys,
                           const uint64_t pte_flags)
{
    assert_msg(!g_mapped_early_identity,
               "mm: mm_early_identity_map_phys() only supports identity "
               "mapping early a single page");

    struct pg_walker walker;
    pgwalker_create_from_root_phys(&walker,
                                   root_phys,
                                   /*virt_addr=*/phys,
                                   pgwalker_early_alloc_pgtable_cb,
                                   /*free_pgtable=*/nullptr);

    g_mapped_early_top_level = walker.level - 1;
    const enum pgwalker_result walker_result =
        pgwalker_fill_in_to(&walker,
                            /*level=*/1,
                            /*should_ref=*/false,
                            /*alloc_pgtable_cb_info=*/nullptr,
                            /*free_pgtable_cb_info=*/nullptr);

    assert_msg(walker_result == E_PGWALKER_OK,
               "mm: failed to fill out pagemap in "
               "mm_early_identity_map_phys()");

    pte_t *const pte = &walker.tables[0][walker.indices[0]];
    pte_write(pte, phys | pte_flags);

    g_mapped_early_phys = phys;
    g_mapped_early_root_phys = root_phys;
    g_mapped_early_identity = true;
}

__debug_optimize(3) void mm_remove_early_identity_map() {
    if (!g_mapped_early_identity) {
        return;
    }

    struct pg_walker walker;
    pgwalker_create_from_root_phys(&walker,
                                   g_mapped_early_root_phys,
                                   /*virt_addr=*/g_mapped_early_phys,
                                   pgwalker_early_alloc_pgtable_cb,
                                   /*free_pgtable=*/nullptr);

    for (pg_level_t level = 1; level <= g_mapped_early_top_level; level++) {
        pte_t *const table = walker.tables[level - 1];

        const uint64_t phys = virt_to_phys(table);
        const uint64_t section_index =
            (uint64_t)(phys_to_section(phys) - mm_get_page_section_list());

        struct page *const page = phys_to_page(phys);

        page->section = section_index + 1;
        page->state = PAGE_STATE_USED;

        free_page(page);
    }
}

__debug_optimize(3)
static void mark_crucial_pages(const struct page_section *const memmap) {
    struct freepage_array_info *iter = nullptr;
    list_foreach(&g_asc_freelist, asc_list, iter) {
        uint64_t iter_phys = virt_to_phys(iter);
        if (!range_has_loc(memmap->range, iter_phys)) {
            continue;
        }

        struct page *const start = phys_to_page(memmap->range.front);

        // Mark the range from the beginning of the memmap, to the first free
        // page as unusable.

        struct page *page = start;
        struct page *const unusable_end = virt_to_page(iter);

        for (; page != unusable_end; page++) {
            page->state = PAGE_STATE_SYSTEM_CRUCIAL;
        }

        while (true) {
            // Mark the range from after the free pages, to the end of the
            // original free-page area.

            page += iter->avail_page_count;
            for (uint64_t i = iter->avail_page_count;
                 i != iter->total_page_count;
                 i++, page++)
            {
                page->state = PAGE_STATE_SYSTEM_CRUCIAL;
            }

            iter = list_next(iter, list);
            if (&iter->list == &g_freepage_list) {
                // Mark the range from after the original free-area, to the end
                // of the memmap.

                const struct page *const memmap_end =
                    start + PAGE_COUNT(memmap->range.size);

                for (; page != memmap_end; page++) {
                    page->state = PAGE_STATE_SYSTEM_CRUCIAL;
                }

                return;
            }

            iter_phys = virt_to_phys(iter);
            if (!range_has_loc(memmap->range, iter_phys)) {
                // Mark the range from after the original free-area, to the end
                // of the memmap.

                const struct page *const memmap_end =
                    start + PAGE_COUNT(memmap->range.size);

                for (; page != memmap_end; page++) {
                    page->state = PAGE_STATE_SYSTEM_CRUCIAL;
                }

                return;
            }

            // Mark all pages in between this iterator and the prior as
            // unusable.

            struct page *const end = phys_to_page(iter_phys);
            for (; page != end; page++) {
                page->state = PAGE_STATE_SYSTEM_CRUCIAL;
            }
        }
    }

    // If we couldn't find a corresponding freepage_list_info struct, then this
    // entire memmap has been used, and needs to be marked as such.

    const uint64_t memmap_page_count = PAGE_COUNT(memmap->range.size);
    struct page *const page = phys_to_page(memmap->range.front);

    for_upto_limit(memmap_page_count, i) {
        page[i].state = PAGE_STATE_SYSTEM_CRUCIAL;
    }
}

__debug_optimize(3) static void assign_section_numbers_to_pages() {
    struct freepage_array_info *iter = nullptr;
    const struct page_section *const section_list =
        mm_get_page_section_list();

    list_foreach(&g_freepage_list, list, iter) {
        uint64_t iter_phys = virt_to_phys(iter);
        uint64_t back_phys =
            virt_to_phys((void *)iter +
                         ((iter->avail_page_count - 1) << PAGE_SHIFT));

        do {
            struct page_section *const section = phys_to_section(iter_phys);
            uint64_t sect_back_phys = back_phys;

            if (!range_has_loc(section->range, sect_back_phys)) {
                sect_back_phys =
                    range_get_end_assert(section->range) - PAGE_SIZE;
            }

            struct page *page = phys_to_page(iter_phys);
            const struct page *const end = phys_to_page(sect_back_phys) + 1;

            const page_section_t section_number = (section - section_list) + 1;
            for (; page != end; page++) {
                page->section = section_number;
            }

            iter_phys = sect_back_phys + PAGE_SIZE;
        } while (iter_phys <= back_phys);
    }
}

__debug_optimize(3) static uint64_t free_all_pages() {
    struct freepage_array_info *iter = nullptr;
    struct freepage_array_info *tmp = nullptr;

    /*
     * Free pages into the buddy allocator while ensuring
     *  (1) The range of pages belong to the same zone.
     *  (2) The buddies of the first page for each order from 0...order are
     *      located after the first page.
     */

    uint64_t free_page_count = 0;
    list_foreach_rev_mut(&g_asc_freelist, asc_list, iter, tmp) {
        uint64_t phys = virt_to_phys(iter);
        uint64_t avail = iter->avail_page_count;

        struct page *page = phys_to_page(phys);
        struct page_section *section = page_to_section(page);

        // iorder is log2() of the number of available pages.
        int8_t iorder = MAX_ORDER - 1;

        do {
            for (; iorder >= 0; iorder--) {
                if (avail >= 1ull << iorder) {
                    break;
                }
            }

            // jorder is the order of pages that all fit in the same zone.
            // jorder should be equal to iorder in most cases, except for when a
            // section crosses the boundary of two zones.

            int8_t jorder = iorder;
            for (; jorder >= 0; jorder--) {
                const auto back_page = page + (1ull << jorder) - 1;
                if (section == page_to_section(back_page)) {
                    break;
                }
            }

            const uint8_t highest_jorder = (uint8_t)jorder;
            if (section->max_order <= highest_jorder) {
                section->max_order = highest_jorder + 1;
            }

            const uint64_t free_count = 1ull << jorder;
            early_free_pages_from_section(page, section, (uint8_t)jorder);

            if (section->min_order > (uint8_t)jorder) {
                section->min_order = (uint8_t)jorder;
            }

            const auto freed_range = RANGE_INIT(phys, free_count << PAGE_SHIFT);
            printk(LOGLEVEL_INFO,
                   "mm: freed %" PRIu64 " pages at " RANGE_FMT " to zone %s\n",
                   free_count,
                   RANGE_FMT_ARGS(freed_range),
                   section->zone->name);

            avail -= free_count;
            if (avail == 0) {
                break;
            }

            page += free_count;
            phys += freed_range.size;
            section = page_to_section(page);
        } while (true);

        list_deinit(&iter->list);
        free_page_count += iter->avail_page_count;
    }

    return free_page_count;
}

extern struct page_section *boot_add_section_at(struct page_section *section);

__debug_optimize(3) static inline void
split_section_at_boundary(struct page_section *const section,
                          struct page_zone *const zone,
                          const uint64_t boundary)
{
    const uint64_t new_section_pfn = section->pfn;
    const struct range new_section_range =
        range_create_end(section->range.front, boundary);

    section->pfn += PAGE_COUNT(new_section_range.size);
    section->range = range_from_loc(section->range, boundary);
    section->zone = phys_to_zone(section->range.front);

    struct page_section *const new_section = boot_add_section_at(section);
    page_section_init(new_section, zone, new_section_range, new_section_pfn);
}

__debug_optimize(3) static inline
uint64_t find_boundary_for_section_split(struct page_section *const section) {
    struct page_zone *const zone = section->zone;

    uint64_t search_front = section->range.front;
    uint64_t mid_index = section->range.size / 2;

    while (mid_index != 0) {
        if (phys_to_zone(search_front + mid_index) == zone) {
            // Continue rightward.
            search_front += mid_index;
        }

        mid_index /= 2;
    }

    return align_up_assert(search_front, PAGE_SIZE);
}

__debug_optimize(3) static inline void split_sections_for_zones() {
    ptrarr_foreach_mut(mm_get_page_section_list(),
                       mm_get_section_count(),
                       section)
    {
        const auto begin_zone = phys_to_zone(section->range.front);
        const auto back_zone =
            phys_to_zone(range_get_end_assert(section->range) - PAGE_SIZE);

        if (section->zone == nullptr) {
            section->zone = begin_zone;
        }

        if (begin_zone == back_zone) {
            continue;
        }

        const uint64_t boundary = find_boundary_for_section_split(section);
        split_section_at_boundary(section, begin_zone, boundary);
    }
}

__debug_optimize(3) static inline void setup_zone_section_list() {
    uint32_t number = 1;
    ptrarr_foreach(mm_get_page_section_list(), mm_get_section_count(), sect) {
        printk(LOGLEVEL_INFO,
               "mm: section %" PRIu32 " at range " RANGE_FMT ", "
               "pfn-range: " RANGE_FMT ", zone: %s\n",
               number,
               RANGE_FMT_ARGS(sect->range),
               RANGE_FMT_ARGS(
                RANGE_INIT(sect->pfn, PAGE_COUNT(sect->range.size))),
               sect->zone->name);

        list_add(&sect->zone->section_list, &sect->zone_list);
        number++;
    }
}

void mm_post_arch_init() {
    // Claim bootloader-reclaimable memmaps now that we've switched to our own
    // pagemap.

    // FIXME: Avoid claiming thees pages until we setup our own stack.

#if 0
    mm_for_each_memmap(memmap) {
        if (memmap->kind == MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE) {
            // Don't claim bootloader-reclaimable memmaps until after we
            // switch to our own pagemap, because there is a slight chance
            // we allocate the root physical page of the bootloader's
            // page tables.

            claim_pages(memmap);
        }
    }
#endif

    // Iterate over the usable-memmaps (sections) 2 times:
    //  1. Iterate to mark used-pages first. This must be done first because
    //     it needs to be done before memmaps are merged, as before the merge,
    //     its obvious which pages are used.
    //  2. Set the section field in page->flags.

    ptrarr_foreach(mm_get_page_section_list(), mm_get_section_count(), sect) {
        mark_crucial_pages(sect);
    }

    split_sections_for_zones();
    setup_zone_section_list();

    assign_section_numbers_to_pages();

    printk(LOGLEVEL_INFO, "mm: finished setting up structpage table\n");
    pagezones_init();

    const uint64_t free_page_count = free_all_pages();
    for_each_page_zone(zone) {
        printk(LOGLEVEL_INFO,
               "mm: zone %s has %" PRIu64 " pages\n",
               zone->name,
               zone->total_free);
    }

    printk(LOGLEVEL_INFO,
           "mm: system has %" PRIu64 " free pages\n",
           free_page_count);

    kmalloc_init();
    physalloc_init();
}