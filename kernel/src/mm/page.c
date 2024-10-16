/*
 * kernel/src/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "lib/overflow.h"

#if defined(__riscv64)
    #include "cpu/info.h"
    #include "sched/thread.h"
#endif /* defined(__riscv64) */

#include "page.h"
#include "section.h"

__debug_optimize(3) void zero_page(void *page) {
#if defined(__x86_64__)
    uint64_t count = PAGE_SIZE / sizeof(uint64_t);
    asm volatile ("cld;\n"
                  "rep stosq"
                  : "+D"(page), "+c"(count)
                  : "a"(0)
                  : "memory");

    return;
#elif defined(__aarch64__)
    asm volatile ("setp [%0]!, %1!, %2;"
                  "setm [%0]!, %1!, %2;"
                  "sete [%0]!, %1!, %2;"
                  :: "r"(page), "r"(PAGE_SIZE), "r"(0ull));

    return;
#elif defined(__riscv64)
    uint64_t cbo_size = 0;
    with_preempt_disabled({
        cbo_size = this_cpu()->cbo_size;
    });

    if (__builtin_expect(cbo_size != 0, 1)) {
        const void *const end = page + PAGE_SIZE;
        for (void *iter = page; iter != end; iter += cbo_size) {
            asm volatile ("cbo.zero (%0)" :: "r"(iter));
        }

        return;
    }
#endif /* defined(__x86_64__) */

    const uint64_t *const end = page + PAGE_SIZE;
    for (uint64_t *iter = page; iter != end; iter++) {
        *iter = 0;
    }
}

__debug_optimize(3) void zero_multiple_pages(void *page, const uint64_t count) {
    const uint64_t full_size = check_mul_assert(PAGE_SIZE, count);
#if defined(__x86_64__)
    uint64_t qword_count = full_size / sizeof(uint64_t);
    asm volatile ("cld;\n"
                  "rep stosq"
                  : "+D"(page), "+c"(qword_count)
                  : "a"(0)
                  : "memory");

    return;
#elif defined(__aarch64__)
    asm volatile ("setp [%0]!, %1!, %2;"
                  "setm [%0]!, %1!, %2;"
                  "sete [%0]!, %1!, %2;"
                  :: "r"(page), "r"(full_size), "r"(0ull));

    return;
#elif defined(__riscv64)
    uint64_t cbo_size = 0;
    with_preempt_disabled({
        cbo_size = this_cpu()->cbo_size;
    });

    if (__builtin_expect(cbo_size != 0, 1)) {
        const void *const end = page + full_size;
        for (void *iter = page; iter != end; iter += cbo_size) {
            asm volatile ("cbo.zero (%0)" :: "r"(iter));
        }

        return;
    }
#endif /* defined(__x86_64__) */

    const uint64_t *const end = page + full_size;
    for (uint64_t *iter = page; iter != end; iter++) {
        *iter = 0;
    }
}

__debug_optimize(3) uint32_t page_get_flags(const struct page *const page) {
    return atomic_load_explicit(&page->flags, memory_order_relaxed);
}

__debug_optimize(3)
void page_set_flag(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_or_explicit(&page->flags, flag, memory_order_relaxed);
}

__debug_optimize(3) bool
page_has_flag(const struct page *const page, const enum struct_page_flags flag)
{
    return page_get_flags(page) & flag;
}

__debug_optimize(3) void
page_clear_flag(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_and_explicit(&page->flags, ~flag, memory_order_relaxed);
}

__debug_optimize(3)
void set_pages_dirty(struct page *const page, const uint64_t amount) {
    struct page *iter = page;
    uint64_t avail = amount;

    do {
        page_set_flag(iter, __PAGE_IS_DIRTY);

        const enum page_state page_state = page_get_state(iter);
        if (page_state == PAGE_STATE_LARGE_HEAD) {
            atomic_store_explicit(&iter->extra.largepage_head.flags,
                                  __PAGE_LARGEHEAD_IS_DIRTY,
                                  memory_order_relaxed);

            const uint8_t order =
                largepage_level_info_list[iter->largehead.level - 1].order;

            struct page *tail = iter + 1;
            const struct page *const end = iter + (1ull << order);

            for (; tail != end; tail++) {
                page_set_flag(tail, __PAGE_IS_DIRTY);
            }

            avail -= min(avail, 1ull << order);
            if (avail == 0) {
                break;
            }

            iter += 1ull << order;
        } else if (page_state == PAGE_STATE_LARGE_TAIL) {
            struct page *const largehead = iter->largetail.head;
            atomic_store_explicit(&largehead->extra.largepage_head.flags,
                                  __PAGE_LARGEHEAD_HAS_DIRTY_PAGE,
                                  memory_order_relaxed);

            iter += 1;
            avail -= 1;
        } else {
            iter += 1;
            avail -= 1;
        }
    } while (avail != 0);
}

__debug_optimize(3)
enum page_state page_get_state(const struct page *const page) {
    return atomic_load_explicit(&page->state, memory_order_relaxed);
}

__debug_optimize(3)
void page_set_state(struct page *const page, const enum page_state state) {
    atomic_store_explicit(&page->state, state, memory_order_relaxed);
}

__debug_optimize(3)
struct page_section *page_to_section(const struct page *const page) {
    assert(page->section != 0);
    return &mm_get_page_section_list()[page->section - 1];
}