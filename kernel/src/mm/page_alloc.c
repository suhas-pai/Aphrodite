/*
 * kernel/src/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>

#include <lib/align.h>
#include <lib/util.h>

#include "dev/printk.h"

#include "mm/page.h"
#include "mm/section.h"
#include "mm/zone.h"

#include "sys/boot.h"
#include "sched/process.h"

// Caller is required to set section->min_order
__debug_optimize(3) static void
add_to_freelist_order(struct page_section *const section,
                      const uint8_t freelist_order,
                      struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST_HEAD);
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    if (freelist_order != 0) {
        struct page *const back = arrptr_back(page, 1ull << freelist_order);

        page_set_state(back, PAGE_STATE_FREE_LIST_TAIL);
        back->freelist_tail.head = page;
    }

    atomic_fetch_add(&section->zone->total_free, 1ull << freelist_order);

    section->total_free += 1ull << freelist_order;
    freelist->count++;
}

// Add pages from the tail pages of a higher order into a lower order
__debug_optimize(3) static void
add_to_freelist_order_from_higher(struct page_section *const section,
                                  const uint8_t freelist_order,
                                  struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST_HEAD);
    page->freelist_head.order = freelist_order;

    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    if (freelist_order != 0) {
        struct page *const back = arrptr_back(page, 1ull << freelist_order);

        back->state = PAGE_STATE_FREE_LIST_TAIL;
        back->freelist_tail.head = page;
    }

    freelist->count++;
}

__debug_optimize(3) static inline struct page *
setup_pages_in_lock(struct page *const page,
                    const uint8_t order,
                    const enum page_state state)
{
    switch (state) {
        case PAGE_STATE_SYSTEM_CRITICAL:
            verify_not_reached();
        case PAGE_STATE_KERNEL_STACK:
        case PAGE_STATE_USER_STACK:
        case PAGE_STATE_USED: {
            arrptr_foreach(page, 1ull << order, iter) {
                page_set_state(iter, state);
            }

            return page;
        }
        case PAGE_STATE_IN_FREE_LIST:
        case PAGE_STATE_FREE_LIST_HEAD:
        case PAGE_STATE_FREE_LIST_TAIL:
        case PAGE_STATE_LRU_CACHE:
            verify_not_reached();
        case PAGE_STATE_SLAB_HEAD: {
            page_set_state(page, state);
            arrptr_foreach(page + 1, (1ull << order) - 1, iter) {
                page_set_state(iter, PAGE_STATE_SLAB_TAIL);
            }

            return page;
        }
        case PAGE_STATE_SLAB_TAIL:
            verify_not_reached();
        case PAGE_STATE_TABLE:
            page_set_state(page, state);
            return page;
        case PAGE_STATE_LARGE_HEAD: {
            page_set_state(page, PAGE_STATE_LARGE_HEAD);
            arrptr_foreach(page + 1, (1ull << order) - 1, iter) {
                page_set_state(iter, PAGE_STATE_LARGE_TAIL);
            }

            return page;
        }
        case PAGE_STATE_LARGE_TAIL:
            verify_not_reached();
        case PAGE_STATE_SIMPLE_ALLOC:
            page_set_state(page, state);
            return page;
    }

    verify_not_reached();
}

__no_sanitize("undefined")
static inline void update_section_max(struct page_section *const section) {
    const struct page_freelist *iter = carr_rbegin(section->freelist_list);
    const struct page_freelist *const end = carr_rend(section->freelist_list);

    for (; iter != end; iter--) {
        if (iter->count != 0) {
            break;
        }
    }

    section->max_order = (iter - section->freelist_list) + 1;
}

__debug_optimize(3) struct page *
take_off_freelist_to_add_later(struct page_section *const section,
                               const uint8_t freelist_order,
                               struct page *const page)
{
    list_deinit(&page->freelist_head.freelist);
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    freelist->count--;
    if (freelist->count != 0) {
        return page;
    }

    if (section->min_order == freelist_order) {
        const struct page_freelist *iter = freelist;
        const struct page_freelist *const end =
            carr_end(section->freelist_list);

        for (; iter != end; iter++) {
            if (iter->count != 0) {
                break;
            }
        }

        section->min_order = iter - section->freelist_list;
    }

    if (section->max_order == freelist_order) {
        update_section_max(section);
    }

    return page;
}

__debug_optimize(3) static struct page *
take_only_order_off_freelist(struct page_section *const section,
                             const uint8_t freelist_order,
                             struct page *const page,
                             const uint8_t remove_page_order,
                             const enum page_state state)
{
    assert(remove_page_order <= freelist_order);
    atomic_fetch_sub(&section->zone->total_free, 1ull << remove_page_order);

    section->total_free -= 1ull << remove_page_order;
    if (section->total_free == 0) {
        section->min_order = 0;
        section->max_order = 0;

        if (page_get_state(page) == PAGE_STATE_FREE_LIST_HEAD) {
            struct page_freelist *const freelist =
                &section->freelist_list[freelist_order];

            list_deinit(&page->freelist_head.freelist);
            freelist->count--;
        }

        return setup_pages_in_lock(page, freelist_order, state);
    }

    take_off_freelist_to_add_later(section, freelist_order, page);
    return setup_pages_in_lock(page, freelist_order, state);
}

__debug_optimize(3) static struct page *
get_from_freelist_order(struct page_section *const section,
                        const uint8_t order,
                        const uint8_t page_remove_order,
                        const enum page_state state)
{
    struct page_freelist *const freelist = &section->freelist_list[order];
    if (freelist->count == 0) {
        return nullptr;
    }

    struct page *const page =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    return take_only_order_off_freelist(section,
                                        order,
                                        page,
                                        page_remove_order,
                                        state);
}

__debug_optimize(3) static void
free_range_of_pages(struct page *page,
                    struct page_section *const section,
                    const uint64_t amount,
                    const uint8_t max_order)
{
    int8_t order = max_order - 1;
    uint64_t avail = amount;

    for (uint8_t order_i = 0; order_i <= max_order; order_i++) {
        if (avail < 1ull << order_i) {
            order = order_i - 1;
            break;
        }
    }

    // max_order in section is one plus the highest order that has at least one
    // page in its freelist.

    uint8_t highest_order = (uint8_t)order;
    if (section->max_order <= highest_order) {
        section->max_order = highest_order + 1;
    }

    do {
        add_to_freelist_order(section, (uint8_t)order, page);

        const uint64_t page_count = 1ull << (uint8_t)order;
        avail -= page_count;

        if (avail == 0) {
            if (section->min_order > (uint8_t)order) {
                section->min_order = (uint8_t)order;
            }

            break;
        }

        page += page_count;
        do {
            if (avail >= 1ull << order) {
                break;
            }

            order--;
        } while (true);
    } while (true);
}

__debug_optimize(3) static void
free_extra_pages_if_from_higher_order(struct page *const page,
                                      struct page_section *const section,
                                      uint8_t allocated_order,
                                      const uint8_t requested_order)
{
    assert(requested_order <= allocated_order);
    if (allocated_order == requested_order) {
        return;
    }

    const uint8_t highest_freed_order = allocated_order - 1;
    if (section->max_order <= highest_freed_order) {
        section->max_order = highest_freed_order + 1;
    }

    while (allocated_order > requested_order) {
        allocated_order--;

        struct page *const buddy_page = page + (1ull << allocated_order);
        add_to_freelist_order_from_higher(section, allocated_order, buddy_page);
    }

    if (section->min_order > allocated_order) {
        section->min_order = allocated_order;
    }
}

__debug_optimize(3) static struct page *
get_from_freelist_order_at_align(struct page_section *const section,
                                 const uint8_t order,
                                 const uint8_t req_order,
                                 const uint8_t align,
                                 const enum page_state state)
{
    struct page_freelist *const freelist = &section->freelist_list[order];
    if (freelist->count == 0) {
        return nullptr;
    }

    struct page *const page =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    const uint64_t phys = page_to_phys(page);
    const uint64_t size = 1ull << order;

    if (has_align(phys, 1ull << align)) {
        free_extra_pages_if_from_higher_order(page, section, order, req_order);
        return take_only_order_off_freelist(section,
                                            order,
                                            page,
                                            req_order,
                                            state);
    }

    const uint64_t index =
        PAGE_COUNT(align_up_assert(phys, 1ull << align) - phys);

    if (!index_in_bounds(index, size)) {
        return nullptr;
    }

    const uint64_t req_page_count = 1ull << req_order;
    const uint64_t remaining = size - index;

    if (remaining < req_page_count) {
        return nullptr;
    }

    if (index != 0) {
        take_off_freelist_to_add_later(section, order, page);
        free_range_of_pages(page,
                            section,
                            /*amount=*/index,
                            /*max_order=*/order);
    }

    struct page *const result = page + index;
    if (index + req_page_count != size) {
        free_range_of_pages(result + req_page_count,
                            section,
                            /*amount=*/size - (index + req_page_count),
                            /*max_order=*/order);
    }

    return result;
}

static void
place_head_range_of_free_pages_in_lower_order(
    struct page *page,
    struct page_section *const section,
    const uint64_t amount,
    const uint8_t orig_order)
{
    int8_t order = orig_order - 1;
    uint64_t avail = amount;

    for (uint8_t order_i = 0; order_i <= orig_order; order_i++) {
        if (avail < 1ull << order_i) {
            order = order_i - 1;
            break;
        }
    }

    list_remove(&page->freelist_head.freelist);
    list_add(&section->freelist_list[order].page_list,
             &page->freelist_head.freelist);

    page->freelist_head.order = (uint8_t)order;
    avail -= 1ull << order;

    if (avail == 0) {
        return;
    }

    free_range_of_pages(page + (1ull << order), section, avail, (uint8_t)order);
}

__debug_optimize(3) static struct page *
get_large_from_freelist_order(struct page_section *const section,
                              const uint8_t freelist_order,
                              const uint8_t large_order)
{
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    if (freelist->count == 0) {
        return nullptr;
    }

    struct page *head =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    struct page *page = head;
    uint64_t page_phys = page_to_phys(page);

    const uint64_t align = PAGE_SIZE << large_order;
    if (large_order != freelist_order) {
        do {
            uint64_t new_phys = page_phys;
            if (!has_align(page_phys, align)) {
                if (align_up(page_phys, align, &new_phys)) {
                    // Try finding a page in this freelist range with the
                    // correct alignment, and if found, take it off the
                    // freelist, and free both the preceding and succeeding
                    // pages back into the freelist.

                    take_only_order_off_freelist(section,
                                                 freelist_order,
                                                 head,
                                                 large_order,
                                                 PAGE_STATE_USED);

                    page += PAGE_COUNT(new_phys - page_phys);
                    page_phys = new_phys;

                    setup_pages_in_lock(page,
                                        large_order,
                                        PAGE_STATE_LARGE_HEAD);

                    const uint64_t free_amount = (uint64_t)(page - head);
                    place_head_range_of_free_pages_in_lower_order(
                        head,
                        section,
                        free_amount,
                        freelist_order);

                    struct page *const begin = page + (1ull << large_order);
                    const struct page *const end =
                        arrptr_end(head, 1ull << freelist_order);

                    const uint64_t count = (uint64_t)(end - begin);
                    if (count != 0) {
                        free_range_of_pages(begin,
                                            section,
                                            count,
                                            freelist_order);
                    }
                }
            } else {
                take_only_order_off_freelist(section,
                                             freelist_order,
                                             head,
                                             large_order,
                                             PAGE_STATE_USED);

                // The head page is correctly aligned, but we've removed a
                // freelist from a higher order, so free the extra pages at the
                // into the freelist.

                free_extra_pages_if_from_higher_order(page,
                                                      section,
                                                      freelist_order,
                                                      large_order);

                setup_pages_in_lock(page, large_order, PAGE_STATE_LARGE_HEAD);
            }

            head = list_next(head, freelist_head.freelist);
            if (&head->freelist_head.freelist == &freelist->page_list) {
                return nullptr;
            }

            page = head;
            page_phys = page_to_phys(page);
        } while (true);
    } else {
        do {
            if (has_align(page_phys, align)) {
                take_only_order_off_freelist(section,
                                             freelist_order,
                                             head,
                                             freelist_order,
                                             PAGE_STATE_LARGE_HEAD);
                break;
            }

            head = list_next(head, freelist_head.freelist);
            if (&head->freelist_head.freelist == &freelist->page_list) {
                return nullptr;
            }

            page = head;
            page_phys = page_to_phys(page);
        } while (true);

        setup_pages_in_lock(page, large_order, PAGE_STATE_LARGE_HEAD);
    }

    return page;
}

__debug_optimize(3) struct page *
setup_pages_for_state(struct page *const page,
                      const enum page_state state,
                      const uint64_t alloc_flags,
                      const uint8_t order,
                      const struct largepage_level_info *const large_info)
{
    switch (state) {
        case PAGE_STATE_SYSTEM_CRITICAL:
            verify_not_reached();
        case PAGE_STATE_USED: {
            const uint64_t page_count = 1ull << order;
            arrptr_foreach(page, page_count, iter) {
                list_init(&page->used.delayed_free_list);
                refcount_init(&page->used.refcount);
            }

            if (alloc_flags & __ALLOC_ZERO) {
                zero_multiple_pages(page_to_virt(page), page_count);
            }

            return page;
        }
        case PAGE_STATE_IN_FREE_LIST:
        case PAGE_STATE_FREE_LIST_HEAD:
        case PAGE_STATE_FREE_LIST_TAIL:
        case PAGE_STATE_LRU_CACHE:
            verify_not_reached();
        case PAGE_STATE_KERNEL_STACK:
            zero_multiple_pages(page_to_virt(page), 1ull << order);
            list_init(&page->kernel_stack.list);

            return page;
        case PAGE_STATE_USER_STACK:
            zero_multiple_pages(page_to_virt(page), 1ull << order);
            list_init(&page->user_stack.list);

            return page;
        case PAGE_STATE_SLAB_HEAD: {
            zero_multiple_pages(page_to_virt(page), 1ull << order);
            list_init(&page->slab.head.slab_list);

            arrptr_foreach(page + 1, (1ull << order) - 1, iter) {
                iter->slab.tail.head = page;
            }

            return page;
        }
        case PAGE_STATE_SLAB_TAIL:
            verify_not_reached();
        case PAGE_STATE_TABLE:
            zero_page(page_to_virt(page));
            list_init(&page->table.delayed_free_list);

            page->table.refcount = REFCOUNT_EMPTY();
            return page;
        case PAGE_STATE_LARGE_HEAD: {
            refcount_init(&page->largehead.refcount);
            refcount_init(&page->largehead.page_refcount);

            list_init(&page->largehead.delayed_free_list);
            page->largehead.level = large_info->level;

            arrptr_foreach(page + 1, (1ull << order) - 1, iter) {
                refcount_init(&iter->largetail.refcount);
                iter->largetail.head = page;
            }

            if (alloc_flags & __ALLOC_ZERO) {
                zero_multiple_pages(page_to_virt(page), 1ull << order);
            }

            return page;
        }
        case PAGE_STATE_LARGE_TAIL:
            verify_not_reached();
        case PAGE_STATE_SIMPLE_ALLOC:
            page_set_state(page, state);
            list_init(&page->simple_alloc.list);

            page->simple_alloc.index = 0;
            page->simple_alloc.refcount = 1;

            return page;
    }

    verify_not_reached();
}

__debug_optimize(3) static struct page *
try_alloc_pages_from_zone(struct page_zone *const zone,
                          const uint8_t order,
                          const uint64_t alloc_flags,
                          const enum page_state state)
{
    struct page *page = nullptr;
    if (__builtin_expect(atomic_load(&zone->total_free) < 1ull << order, 0)) {
        return nullptr;
    }

    // Iterate over each section and try to acquire the section's lock.
    // Immediately continue if someone is currently holding the lock.

    int flag = 0;

    uint8_t section_index = 0;
    uint8_t alloced_order = 0;

    uint64_t locked_section_mask = 0;
    struct page_section *iter =
        list_head(&zone->section_list, typeof(*iter), zone_list);

    do {
        if (!spin_try_acquire_save_intr(&iter->lock, &flag)) {
            iter = list_next(iter, zone_list);
            if (&iter->zone_list == &zone->section_list) {
                if (locked_section_mask == mm_get_full_section_mask()) {
                    return nullptr;
                }

                iter = list_head(&zone->section_list, typeof(*iter), zone_list);
                section_index = 0;
            }

            continue;
        }

        alloced_order = max(order, iter->min_order);
        const uint8_t max_order = iter->max_order;

        for (; alloced_order < max_order; alloced_order++) {
            page = get_from_freelist_order(iter, alloced_order, order, state);
            if (page != nullptr) {
                goto done;
            }
        }

        spin_release_restore_intr(&iter->lock, flag);

        iter = list_next(iter, zone_list);
        if (&iter->zone_list == &zone->section_list) {
            break;
        }

        locked_section_mask |= 1ull << section_index;
        section_index++;
    } while (true);

    return nullptr;

done:
    free_extra_pages_if_from_higher_order(page, iter, alloced_order, order);

    spin_release_restore_intr(&iter->lock, flag);
    setup_pages_for_state(page,
                          state,
                          alloc_flags,
                          order,
                          /*large_info=*/nullptr);

    return page;
}

__debug_optimize(3) static struct page *
try_alloc_pages_from_zone_at_align(struct page_zone *const zone,
                                   const uint8_t order,
                                   const uint8_t align,
                                   const uint64_t alloc_flags,
                                   const enum page_state state)
{
    struct page *page = nullptr;
    if (__builtin_expect(atomic_load(&zone->total_free) < 1ull << order, 0)) {
        return nullptr;
    }

    // Iterate over each section and try to acquire the section's lock.
    // Immediately continue if someone is currently holding the lock.

    int flag = 0;

    uint8_t section_index = 0;
    uint8_t order_i = 0;

    uint64_t locked_section_mask = 0;
    struct page_section *iter =
        list_head(&zone->section_list, typeof(*iter), zone_list);

    do {
        if (!spin_try_acquire_save_intr(&iter->lock, &flag)) {
            iter = list_next(iter, zone_list);
            if (&iter->zone_list == &zone->section_list) {
                if (locked_section_mask == mm_get_full_section_mask()) {
                    return nullptr;
                }

                iter = list_head(&zone->section_list, typeof(*iter), zone_list);
                section_index = 0;
            }

            continue;
        }

        order_i = max(order, iter->min_order);
        const uint8_t max_order = iter->max_order;

        for (; order_i < max_order; order_i++) {
            page =
                get_from_freelist_order_at_align(iter,
                                                 /*order=*/order_i,
                                                 /*orig_order=*/order,
                                                 align,
                                                 state);

            if (page != nullptr) {
                goto done;
            }
        }

        spin_release_restore_intr(&iter->lock, flag);

        iter = list_next(iter, zone_list);
        if (&iter->zone_list == &zone->section_list) {
            break;
        }

        locked_section_mask |= 1ull << section_index;
        section_index++;
    } while (true);

    return nullptr;

done:
    spin_release_restore_intr(&iter->lock, flag);
    setup_pages_for_state(page,
                          state,
                          alloc_flags,
                          order,
                          /*large_info=*/nullptr);

    return page;
}

struct page *
alloc_pages(const enum page_state state,
            const uint64_t alloc_flags,
            const uint8_t order)
{
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return nullptr;
    }

    struct page_zone *zone = page_zone_default();
    struct page *page = nullptr;

    while (zone != nullptr) {
        page = try_alloc_pages_from_zone(zone, order, alloc_flags, state);
        if (page != nullptr) {
            return page;
        }

        zone = zone->fallback_zone;
    }

    return nullptr;
}

struct page *
alloc_pages_count(const enum page_state state,
                  const uint64_t alloc_flags,
                  const uint16_t count,
                  uint16_t *const count_out)
{
    uint8_t order = 0;
    while ((1ull << order) < count && order < MAX_ORDER) {
        order++;
    }

    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_pages_count() got count >= MAX_ORDER\n");

        return nullptr;
    }

    struct page *const page = alloc_pages(state, alloc_flags, order);
    if (page == nullptr) {
        return nullptr;
    }

    *count_out = 1ull << order;
    return page;
}

struct page *
alloc_pages_from_zone(struct page_zone *zone,
                      const enum page_state state,
                      const uint64_t alloc_flags,
                      const uint8_t order,
                      const bool allow_fallback)
{
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return nullptr;
    }

    struct page *page =
        try_alloc_pages_from_zone(zone, order, alloc_flags, state);

    if (page != nullptr) {
        return page;
    }

    if (!allow_fallback) {
        return nullptr;
    }

    do {
        zone = zone->fallback_zone;
        if (zone == nullptr) {
            break;
        }

        page = try_alloc_pages_from_zone(zone, order, alloc_flags, state);
        if (page != nullptr) {
            return page;
        }

    } while (true);

    return nullptr;
}

struct page *
alloc_pages_at_align(const enum page_state state,
                     const uint64_t alloc_flags,
                     const uint8_t align,
                     const uint8_t order)
{
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return nullptr;
    }

    struct page_zone *zone = page_zone_default();
    struct page *page = nullptr;

    while (zone != nullptr) {
        page =
            try_alloc_pages_from_zone_at_align(zone,
                                               order,
                                               align,
                                               alloc_flags,
                                               state);

        if (page != nullptr) {
            return page;
        }

        zone = zone->fallback_zone;
    }

    return nullptr;
}

struct page *
alloc_pages_from_zone_at_align(struct page_zone *zone,
                               const enum page_state state,
                               const uint64_t alloc_flags,
                               const uint8_t order,
                               const uint8_t align,
                               const bool allow_fallback)
{
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return nullptr;
    }

    struct page *page =
        try_alloc_pages_from_zone_at_align(zone,
                                           order,
                                           align,
                                           alloc_flags,
                                           state);

    if (page != nullptr) {
        return page;
    }

    if (!allow_fallback) {
        return nullptr;
    }

    do {
        zone = zone->fallback_zone;
        if (zone == nullptr) {
            break;
        }

        page =
            try_alloc_pages_from_zone_at_align(zone,
                                               order,
                                               align,
                                               alloc_flags,
                                               state);

        if (page != nullptr) {
            return page;
        }
    } while (true);

    return nullptr;
}

__debug_optimize(3) static struct page *
try_alloc_large_page_from_zone(struct page_zone *const zone,
                               const struct largepage_level_info *const info)
{
    const uint8_t order = info->order;
    if (__builtin_expect(atomic_load(&zone->total_free) < 1ull << order, 0)) {
        return nullptr;
    }

    struct page_section *iter = nullptr;
    list_foreach(&zone->section_list, zone_list, iter) {
        int flag = 0;
        if (!spin_try_acquire_save_intr(&iter->lock, &flag)) {
            continue;
        }

        uint8_t alloced_order = max(order, iter->min_order);
        const uint8_t max_order = iter->max_order;

        for (; alloced_order < max_order; alloced_order++) {
            struct page *const page =
                get_large_from_freelist_order(iter,
                                              alloced_order,
                                              /*largepage_order=*/order);

            if (page != nullptr) {
                spin_release_restore_intr(&iter->lock, flag);
                return page;
            }
        }

        spin_release_restore_intr(&iter->lock, flag);
    }

    return nullptr;
}

struct page *
alloc_large_page(const pg_level_t level, const uint64_t alloc_flags) {
    struct page_zone *zone = page_zoneiter_start();
    const struct largepage_level_info *const info =
        &lg_page_level_info_list[level - 1];

    if (__builtin_expect(!info->is_supported, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() allocating at level %" PRIu8 " is not "
               "supported\n",
               level);
        return nullptr;
    }

    const uint8_t order = info->order;
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() can't allocate large-page, too large\n");
        return nullptr;
    }

    struct page *page = nullptr;
    while (zone != nullptr) {
        page = try_alloc_large_page_from_zone(zone, info);
        if (page != nullptr) {
            return setup_pages_for_state(page,
                                         PAGE_STATE_LARGE_HEAD,
                                         alloc_flags,
                                         order,
                                         info);
        }

        zone = zone->fallback_zone;
    }

    return nullptr;
}

struct page *
alloc_large_page_from_zone(struct page_zone *zone,
                           const uint64_t alloc_flags,
                           const pg_level_t level,
                           const bool fallback)
{
    const struct largepage_level_info *const info =
        &lg_page_level_info_list[level - 1];

    if (__builtin_expect(!info->is_supported, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() allocating at level %" PRIu8 " is not "
               "supported\n",
               level);
        return nullptr;
    }

    const uint8_t order = info->order;
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() can't allocate large-page, too large\n");
        return nullptr;
    }

    struct page *page = try_alloc_large_page_from_zone(zone, info);
    if (page != nullptr) {
    setup:
        return setup_pages_for_state(page,
                                     PAGE_STATE_LARGE_HEAD,
                                     alloc_flags,
                                     order,
                                     info);
    }

    if (!fallback) {
        return nullptr;
    }

    do {
        zone = zone->fallback_zone;
        if (zone == nullptr) {
            break;
        }

        page = try_alloc_large_page_from_zone(zone, info);
        if (page != nullptr) {
            goto setup;
        }
    } while (true);

    return nullptr;
}

__debug_optimize(3) void
early_free_pages_from_section(struct page *const page,
                              struct page_section *const section,
                              const uint8_t order)
{
    struct page_freelist *const freelist = &section->freelist_list[order];

    page->state = PAGE_STATE_FREE_LIST_HEAD;
    page->freelist_head.order = order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    const uint64_t count = 1ull << order;
    if (order != 0) {
        struct page *const back = arrptr_back(page, count);

        back->state = PAGE_STATE_FREE_LIST_TAIL;
        back->freelist_tail.head = page;
    }

    section->zone->total_free += count;
    section->total_free += count;

    freelist->count++;
}

__debug_optimize(3) bool
find_nearby_free_pages(struct page *const page,
                       uint64_t amount,
                       struct page **const page_out,
                       uint64_t *const amount_out)
{
    struct page_section *const section = page_to_section(page);
    uint64_t page_pfn = page_to_pfn(page);

    struct page *free_page = page;
    bool merged_range = false;

    if (__builtin_expect(page_pfn != section->pfn, 1)) {
        const enum page_state prev_state = page_get_state(free_page - 1);
        if (prev_state == PAGE_STATE_FREE_LIST_TAIL) {
            struct page *const free_head = (free_page - 1)->freelist_tail.head;

            const uint64_t extra = (uint64_t)(page - free_page);
            const uint8_t order = free_head->freelist_head.order;

            // Only take off freelist if we can add the combined range to a
            // higher order.

            if (__builtin_expect(order < MAX_ORDER - 1, 1) &&
                (1ull << order) + extra >= (1ull << (order + 1)))
            {
                take_off_freelist_to_add_later(section, order, free_head);

                free_page = free_head;
                page_pfn -= extra;
                amount += extra;

                merged_range = true;
            }
        } else if (prev_state == PAGE_STATE_FREE_LIST_HEAD) {
            struct page *const free_head = free_page - 1;
            const uint8_t order = free_head->freelist_head.order;

            // We can only take off freelist when we can add the combined range
            // to a higher order if the order is zero.

            if (order == 0) {
                take_off_freelist_to_add_later(section, order, free_head);

                free_page = free_head;
                page_pfn -= 1;
                amount += 1;

                merged_range = true;
            }
        }
    }

    const uint64_t end_pfn = page_pfn + amount;
    const uint64_t section_end_pfn =
        section->pfn + PAGE_COUNT(section->range.size);

    if (__builtin_expect(end_pfn != section_end_pfn, 1)) {
        struct page *const next_page = free_page + amount;
        if (page_get_state(next_page) == PAGE_STATE_FREE_LIST_HEAD) {
            const uint8_t order = next_page->freelist_head.order;

            // Only take off freelist if we can add the combined range to a
            // higher order.

            if ((1ull << order) + amount >= (1ull << (order + 1))) {
                take_off_freelist_to_add_later(section, order, next_page);

                amount += 1ull << order;
                merged_range = true;
            }
        }
    }

    *page_out = free_page;
    *amount_out = amount;

    return merged_range;
}

__debug_optimize(3)
void free_amount_of_pages(struct page *page, uint64_t amount) {
    assert(amount != 0);

    find_nearby_free_pages(page, amount, &page, &amount);
    free_range_of_pages(page, page_to_section(page), amount, MAX_ORDER);
}

void free_large_page(struct page *const head) {
    assert(page_get_state(head) == PAGE_STATE_LARGE_HEAD);

    struct page_section *const section = page_to_section(head);
    with_spinlock_intr_disabled(&section->lock, {
        const pg_level_t level = head->largehead.level;
        struct largepage_level_info *const level_info =
            &lg_page_level_info_list[level - 1];

        const uint64_t page_count = 1ull << level_info->order;
        const struct page *const end = head + page_count;

        ptrrange_foreach(head, end, page) {
            if (!ref_down(&page->largehead.page_refcount)) {
                page++;
                continue;
            }

            struct page *iter = page + 1;
            for (; iter != end; iter++) {
                if (!ref_down(&iter->largehead.page_refcount)) {
                    break;
                }
            }

            free_amount_of_pages(page, (uint64_t)(iter - page));
            page = iter + 1;
        }
    });
}

void free_pages(struct page *page, const uint8_t order) {
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN, "mm: free_pages() got order >= MAX_ORDER\n");
        return;
    }

    if (page_get_state(page) == PAGE_STATE_LARGE_HEAD) {
        if (order != 0) {
            printk(LOGLEVEL_WARN,
                   "mm: free_pages() got order > 0, which is not supported for "
                   "large pages\n");
            return;
        }

        free_large_page(page);
        return;
    }

    struct page_section *section = page_to_section(page);
    uint64_t amount = 1ull << order;

    if (find_nearby_free_pages(page, amount, &page, &amount)) {
        free_range_of_pages(page, section, amount, MAX_ORDER);
    } else {
        add_to_freelist_order(section, order, page);
    }
}

__debug_optimize(3)
struct page *deref_page(struct page *page, struct pageop *const pageop) {
    if (ref_down(&page->used.refcount)) {
        list_add(&pageop->delayed_free, &page->used.delayed_free_list);
        return nullptr;
    }

    return page;
}

struct page *
deref_large_page(struct page *const page,
                 struct pageop *const pageop,
                 const pg_level_t level)
{
    if (page_get_state(page) == PAGE_STATE_LARGE_HEAD) {
        if (ref_down(&page->largehead.refcount)) {
            list_add(&pageop->delayed_free, &page->used.delayed_free_list);
            return nullptr;
        }

        return page;
    }

    const uint64_t count = 1ull << lg_page_level_info_list[level - 1].order;
    arrptr_foreach(page, count, iter) {
        deref_page(iter, pageop);
    }

    return nullptr;
}

__debug_optimize(3) struct page *alloc_table() {
    return alloc_page(PAGE_STATE_TABLE, __ALLOC_ZERO);
}

__debug_optimize(3)
struct page *alloc_user_stack(struct process *const proc, const uint8_t order) {
    struct page *const page =
        alloc_pages(PAGE_STATE_USER_STACK, __ALLOC_ZERO, order);

    if (page == nullptr) {
        return nullptr;
    }

    page->user_stack.process = proc;
    list_add(&proc->stack_page_list, &page->user_stack.list);

    return page;
}