/*
 * kernel/src/mm/slab.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/overflow.h"
#include "lib/string.h"
#include "lib/util.h"

#include "page_alloc.h"

struct free_slab_object {
    uint32_t magic;
    uint32_t next;
};

#define FREE_SLAB_MAGIC 0xcafefeed
#define SLAB_ALIGNMENT 16

_Static_assert(sizeof(struct free_slab_object) <= SLAB_ALIGNMENT,
               "free_slab_object struct must not be greater than "
               "SLAB_ALIGNMENT");

#define MIN_OBJ_PER_SLAB 4

__optimize(3) static uint64_t
get_free_obj_byte_index(struct slab_allocator *const alloc,
                        const uint32_t index)
{
    return check_mul_assert(index, alloc->object_size);
}

__optimize(3) static inline struct free_slab_object *
get_free_ptr_from_index(struct page *const page,
                        struct slab_allocator *const alloc,
                        const uint32_t index)
{
    if (index == UINT32_MAX) {
        return NULL;
    }

    return page_to_virt(page) + get_free_obj_byte_index(alloc, index);
}

__optimize(3) static inline struct free_slab_object *
get_free_ptr(struct page *const page, struct slab_allocator *const alloc) {
    return get_free_ptr_from_index(page,
                                   alloc,
                                   page->slab.head.first_free_index);
}

void slab_verify(struct slab_allocator *const alloc) {
#if defined(CHECK_SLABS)
    if (list_empty(&alloc->free_slab_head_list)) {
        return;
    }

    struct page *const head =
        list_head(&alloc->free_slab_head_list,
                  struct page,
                  slab.head.slab_list);

    assert(index_in_bounds(head->slab.head.first_free_index,
                           alloc->object_count_per_slab));

    struct free_slab_object *free_obj = get_free_ptr(head, alloc);
    uint32_t prev = UINT32_MAX;

    while (free_obj != NULL) {
        if (free_obj->next == UINT32_MAX) {
            break;
        }

        assert(free_obj->magic == FREE_SLAB_MAGIC);
        assert(index_in_bounds(free_obj->next, alloc->object_count_per_slab));
        assert(free_obj->next != prev);

        prev = free_obj->next;
        free_obj = get_free_ptr_from_index(head, alloc, free_obj->next);
    }
#else
    (void)alloc;
#endif /* defined(CHECK_SLABS) */
}

bool
slab_allocator_init(struct slab_allocator *const slab_alloc,
                    const uint32_t object_size_arg,
                    const uint32_t alloc_flags,
                    const uint16_t flags)
{
    uint64_t object_size = object_size_arg;

    // To store free_page_objects, every object must be at least 16 bytes.
    // We also make this the required minimum alignment.

    if (!align_up(object_size, SLAB_ALIGNMENT, &object_size)
        || object_size > UINT32_MAX)
    {
        return false;
    }

    uint32_t min_size_for_slab = object_size;
    if (!check_mul(object_size, MIN_OBJ_PER_SLAB, &min_size_for_slab)) {
        return false;
    }

    list_init(&slab_alloc->free_slab_head_list);

    slab_alloc->lock = SPINLOCK_INIT();
    slab_alloc->object_size = object_size;
    slab_alloc->free_obj_count = 0;
    slab_alloc->alloc_flags = alloc_flags;
    slab_alloc->flags = flags;

    uint8_t order = 0;
    for (; (PAGE_SIZE << order) < min_size_for_slab; order++) {}

    slab_alloc->slab_order = order;
    slab_alloc->object_count_per_slab = (PAGE_SIZE << order) / object_size;

    return true;
}

static struct page *alloc_slab_page(struct slab_allocator *const alloc) {
    struct page *const head =
        alloc_pages(PAGE_STATE_SLAB_HEAD,
                    __ALLOC_ZERO | alloc->alloc_flags,
                    alloc->slab_order);

    if (__builtin_expect(head == NULL, 0)) {
        return NULL;
    }

    const struct page *const end = head + ((1 << alloc->slab_order) - 1);
    for (struct page *page = head + 1; page < end; page++) {
        page->slab.allocator = alloc;
    }

    list_add(&alloc->free_slab_head_list, &head->slab.head.slab_list);

    head->slab.allocator = alloc;
    head->slab.head.first_free_index = 0;
    head->slab.head.free_obj_count = alloc->object_count_per_slab;

    alloc->slab_count++;
    alloc->free_obj_count += alloc->object_count_per_slab;

    void *const head_virt = page_to_virt(head);
    uint64_t object_byte_index = 0;

    for (uint32_t i = 0;
         i != alloc->object_count_per_slab - 1;
         i++, object_byte_index += alloc->object_size)
    {
        struct free_slab_object *const free_obj =
            (struct free_slab_object *)(head_virt + object_byte_index);

        free_obj->magic = FREE_SLAB_MAGIC;
        free_obj->next = i + 1;
    }

    struct free_slab_object *const last_object = head_virt + object_byte_index;

    last_object->magic = FREE_SLAB_MAGIC;
    last_object->next = UINT32_MAX;

    return head;
}

void *slab_alloc(struct slab_allocator *const alloc) {
    slab_verify(alloc);
    int flag = 0;

    const bool needs_lock = (alloc->flags & __SLAB_ALLOC_NO_LOCK) == 0;
    if (needs_lock) {
        flag = spin_acquire_save_irq(&alloc->lock);
    }

    struct page *head = NULL;
    if (list_empty(&alloc->free_slab_head_list)) {
        head = alloc_slab_page(alloc);
        if (head == NULL) {
            if (needs_lock) {
                spin_release_restore_irq(&alloc->lock, flag);
            }

            return NULL;
        }
    } else {
        head =
            list_head(&alloc->free_slab_head_list,
                      struct page,
                      slab.head.slab_list);
    }

    alloc->free_obj_count--;
    head->slab.head.free_obj_count--;

    if (head->slab.head.free_obj_count == 0) {
        list_deinit(&head->slab.head.slab_list);
    }

    struct free_slab_object *const result = get_free_ptr(head, alloc);
    head->slab.head.first_free_index = result ? result->next : UINT32_MAX;

    if (needs_lock) {
        spin_release_restore_irq(&alloc->lock, flag);
    }

    // Zero-out free-block
    result->magic = 0;
    result->next = 0;

    return result;
}

struct page *
slab_alloc2(struct slab_allocator *const alloc, uint64_t *const offset) {
    slab_verify(alloc);
    int flag = 0;

    const bool needs_lock = (alloc->flags & __SLAB_ALLOC_NO_LOCK) == 0;
    if (needs_lock) {
        flag = spin_acquire_save_irq(&alloc->lock);
    }

    struct page *head = NULL;
    if (list_empty(&alloc->free_slab_head_list)) {
        head = alloc_slab_page(alloc);
        if (head == NULL) {
            if (needs_lock) {
                spin_release_restore_irq(&alloc->lock, flag);
            }

            return NULL;
        }
    } else {
        head =
            list_head(&alloc->free_slab_head_list,
                      struct page,
                      slab.head.slab_list);
    }

    alloc->free_obj_count--;
    head->slab.head.free_obj_count--;

    if (head->slab.head.free_obj_count == 0) {
        list_deinit(&head->slab.head.slab_list);
    }

    *offset = get_free_obj_byte_index(alloc, head->slab.head.first_free_index);

    struct free_slab_object *const result = page_to_virt(head) + *offset;
    head->slab.head.first_free_index = result ? result->next : UINT32_MAX;

    if (needs_lock) {
        spin_release_restore_irq(&alloc->lock, flag);
    }

    // Zero-out free-block
    result->magic = 0;
    result->next = 0;

    return head;
}

__optimize(3) static inline struct page *slab_head_of(const void *const mem) {
    struct page *const page = virt_to_page(mem);
    const enum page_state state = page_get_state(page);

    if (state == PAGE_STATE_SLAB_HEAD) {
        return page;
    }

    return page->slab.tail.head;
}

__optimize(3) static inline uint64_t
index_of_free_object(struct page *const head,
                     struct slab_allocator *const alloc,
                     struct free_slab_object *const free_object)
{
    return distance(page_to_virt(head), free_object) / alloc->object_size;
}

void slab_free(void *const mem) {
    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const alloc = head->slab.allocator;

    bzero(mem, alloc->object_size);
    slab_verify(alloc);

    int flag = 0;
    const bool needs_lock = (alloc->flags & __SLAB_ALLOC_NO_LOCK) == 0;

    if (needs_lock) {
        flag = spin_acquire_save_irq(&alloc->lock);
    }

    alloc->free_obj_count += 1;
    head->slab.head.free_obj_count += 1;

    if (head->slab.head.free_obj_count != 1) {
        if (head->slab.head.free_obj_count == alloc->object_count_per_slab) {
            alloc->free_obj_count -= alloc->object_count_per_slab;
            alloc->slab_count -= 1;

            list_deinit(&head->slab.head.slab_list);
            free_pages(head, alloc->slab_order);

            if (needs_lock) {
                spin_release_restore_irq(&alloc->lock, flag);
            }

            return;
        }
    } else {
        // This was previously a fully used slab, so we have to add this back
        // to our list of slabs.

        list_add(&alloc->free_slab_head_list, &head->slab.head.slab_list);
    }

    struct free_slab_object *const free_obj = (struct free_slab_object *)mem;

    free_obj->magic = FREE_SLAB_MAGIC;
    free_obj->next = head->slab.head.first_free_index;

    head->slab.head.first_free_index =
        index_of_free_object(head, alloc, free_obj);

    if (needs_lock) {
        spin_release_restore_irq(&alloc->lock, flag);
    }
}

__optimize(3) uint32_t slab_object_size(void *const mem) {
    if (__builtin_expect(mem == NULL, 0)) {
        panic("slab_object_size(): Got mem=NULL");
    }

    struct page *const page = virt_to_page(mem);
    const enum page_state state = page_get_state(page);

    assert(state == PAGE_STATE_SLAB_HEAD || state == PAGE_STATE_SLAB_TAIL);

    struct slab_allocator *const allocator = page->slab.allocator;
    return allocator->object_size;
}