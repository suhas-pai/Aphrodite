/*
 * kernel/mm/slab.c
 * © suhas pai
 */

#include "cpu/panic.h"
#include "cpu/spinlock.h"

#include "lib/align.h"
#include "lib/overflow.h"
#include "lib/string.h"

#include "mm/page.h"
#include "mm/page_alloc.h"

#include "slab.h"

struct free_slab_object {
    uint32_t next;
};

#define MIN_OBJ_PER_SLAB 4

bool
slab_allocator_init(struct slab_allocator *const slab_alloc,
                    const uint32_t object_size_arg,
                    const uint32_t alloc_flags,
                    const uint16_t flags)
{
    uint64_t object_size = object_size_arg;

    // To store free_page_objects, every object must be at least 16 bytes.
    // We also make this the required minimum alignment.

    if (!align_up(object_size, /*boundary=*/16, &object_size)) {
        return false;
    }

    list_init(&slab_alloc->slab_head_list);

    slab_alloc->lock = SPINLOCK_INIT();
    slab_alloc->object_size = object_size;
    slab_alloc->free_obj_count = 0;
    slab_alloc->alloc_flags = alloc_flags;
    slab_alloc->flags = flags;

    uint16_t order = 0;
    const uint64_t min_size_for_slab = object_size * MIN_OBJ_PER_SLAB;

    for (; (PAGE_SIZE << order) < min_size_for_slab; order++) {}

    slab_alloc->slab_order = order;
    slab_alloc->object_count_per_slab = (PAGE_SIZE << order) / object_size;

    return true;
}

static struct page *alloc_slab_page(struct slab_allocator *const alloc) {
    struct page *const head =
        alloc_pages(PAGE_STATE_SLAB_HEAD, __ALLOC_ZERO, alloc->slab_order);

    if (__builtin_expect(head == NULL, 0)) {
        return NULL;
    }

    const struct page *const end = head + ((1 << alloc->slab_order) - 1);
    for (struct page *page = head + 1; page < end; page++) {
        page->slab.allocator = alloc;
    }

    list_add(&alloc->slab_head_list, &head->slab.head.slab_list);

    head->slab.allocator = alloc;
    head->slab.head.first_free_index = 0;
    head->slab.head.free_obj_count = alloc->object_count_per_slab;

    alloc->slab_count++;
    alloc->free_obj_count += alloc->object_count_per_slab;

    void *const page_virt = page_to_virt(head);
    uint64_t object_byte_index = 0;

    for (uint32_t i = 0;
         i != alloc->object_count_per_slab - 1;
         i++, object_byte_index += alloc->object_size)
    {
        struct free_slab_object *const free_obj =
            (struct free_slab_object *)(page_virt + object_byte_index);

        free_obj->next = i + 1;
    }

    struct free_slab_object *const last_object = page_virt + object_byte_index;
    last_object->next = UINT32_MAX;

    return head;
}

static inline uint64_t
get_free_index(struct page *const page,
               struct slab_allocator *const alloc,
               struct free_slab_object *const free_object)
{
    return distance(page_to_virt(page), free_object) / alloc->object_size;
}

static inline void *
get_free_ptr(struct page *const page, struct slab_allocator *const alloc) {
    if (page->slab.head.first_free_index == UINT32_MAX) {
        return NULL;
    }

    const uint64_t byte_index =
        check_mul_assert(page->slab.head.first_free_index, alloc->object_size);

    return page_to_virt(page) + byte_index;
}

void *slab_alloc(struct slab_allocator *const alloc) {
    int flag = 0;

    const bool needs_lock = (alloc->flags & __SLAB_ALLOC_NO_LOCK) == 0;
    if (needs_lock) {
        flag = spin_acquire_with_irq(&alloc->lock);
    }

    struct page *page = NULL;
    if (list_empty(&alloc->slab_head_list)) {
        page = alloc_slab_page(alloc);
        if (page == NULL) {
            if (needs_lock) {
                spin_release_with_irq(&alloc->lock, flag);
            }

            return NULL;
        }
    } else {
        page =
            list_head(&alloc->slab_head_list, struct page, slab.head.slab_list);
    }

    alloc->free_obj_count--;
    page->slab.head.free_obj_count--;

    if (page->slab.head.free_obj_count == 0) {
        list_delete(&page->slab.head.slab_list);
    }

    struct free_slab_object *const result = get_free_ptr(page, alloc);
    page->slab.head.first_free_index = result ? result->next : UINT32_MAX;

    if (needs_lock) {
        spin_release_with_irq(&alloc->lock, flag);
    }

    return result;
}

__optimize(3) static inline struct page *slab_head_of(const void *const mem) {
    struct page *const page = virt_to_page(mem);
    if (page_get_state(page) == PAGE_STATE_SLAB_HEAD) {
        return page;
    }

    return page->slab.tail.head;
}

void slab_free(void *const mem) {
    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const alloc = head->slab.allocator;

    bzero(mem, alloc->object_size);

    int flag = 0;
    const bool needs_lock = (alloc->flags & __SLAB_ALLOC_NO_LOCK) == 0;

    if (needs_lock) {
        flag = spin_acquire_with_irq(&alloc->lock);
    }

    alloc->free_obj_count += 1;
    head->slab.head.free_obj_count += 1;

    if (head->slab.head.free_obj_count != 1) {
        if (head->slab.head.free_obj_count == alloc->object_count_per_slab) {
            list_delete(&head->slab.head.slab_list);

            alloc->free_obj_count -= alloc->object_count_per_slab;
            alloc->slab_count -= 1;

            free_pages(head, alloc->slab_order);
            if (needs_lock) {
                spin_release_with_irq(&alloc->lock, flag);
            }

            return;
        }
    } else {
        // This was previously a fully used slab, so we have to add this back
        // to our list of slabs.

        list_add(&alloc->slab_head_list, &head->slab.head.slab_list);
    }

    struct free_slab_object *const free_obj = (struct free_slab_object *)mem;

    free_obj->next = head->slab.head.first_free_index;
    head->slab.head.first_free_index = get_free_index(head, alloc, mem);

    if (needs_lock) {
        spin_release_with_irq(&alloc->lock, flag);
    }
}

__optimize(3) uint32_t slab_object_size(void *const mem) {
    if (__builtin_expect(mem == NULL, 0)) {
        panic("slab_object_size(): Got mem=NULL");
    }

    struct page *const page = virt_to_page(mem);
    const enum page_state state = page_get_state(page);

    assert(
        __builtin_expect(state == PAGE_STATE_SLAB_HEAD ||
                         state == PAGE_STATE_SLAB_TAIL, 1));

    struct slab_allocator *const allocator = page->slab.allocator;
    return allocator->object_size;
}