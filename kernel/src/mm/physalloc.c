/*
 * kernel/src/mm/physalloc.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "mm/mm_types.h"
#include "mm/physalloc.h"
#include "mm/slab.h"

static struct slab_allocator physalloc_slabs[14] = {0};
static bool physalloc_is_initialized = false;

__debug_optimize(3) bool physalloc_initialized() {
    return physalloc_is_initialized;
}

#define SLAB_ALLOC_INIT(size, alloc_flags, flags) \
    assert( \
        slab_allocator_init(&physalloc_slabs[index], \
                            size, \
                            alloc_flags, \
                            flags)); \
    index++;

void physalloc_init() {
    uint8_t index = 0;

    SLAB_ALLOC_INIT(256, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(384, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(512, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(768, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(1024, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(1536, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(2048, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(3072, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(4096, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(6102, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(8192, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(16384, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(32768, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(65536, /*alloc_flags=*/0, /*flags=*/0);

    physalloc_is_initialized = true;
}

__debug_optimize(3) uint64_t physalloc(const uint32_t size) {
    assert_msg(physalloc_is_initialized,
               "mm: physalloc() called before physalloc_init()");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: physalloc() got size=0\n");
        return INVALID_PHYS;
    }

    if (__builtin_expect(size > PHALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: physalloc() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               PHALLOC_MAX);
        return INVALID_PHYS;
    }

    struct slab_allocator *allocator = &physalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    uint64_t offset = 0;
    struct page *const page = slab_alloc2(allocator, &offset);

    if (page == NULL) {
        return INVALID_PHYS;
    }

    return page_to_phys(page) + offset;
}

__debug_optimize(3)
uint64_t physalloc_size(const uint32_t size, uint32_t *const size_out) {
    assert_msg(physalloc_is_initialized,
               "mm: physalloc() called before physalloc_init()");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: physalloc() got size=0\n");
        return INVALID_PHYS;
    }

    if (__builtin_expect(size > PHALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: physalloc() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               PHALLOC_MAX);
        return INVALID_PHYS;
    }

    struct slab_allocator *allocator = &physalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    uint64_t offset = 0;
    struct page *const page = slab_alloc2(allocator, &offset);

    if (page == NULL) {
        return INVALID_PHYS;
    }

    *size_out = allocator->object_size;
    return page_to_phys(page) + offset;
}

__debug_optimize(3)
uint64_t physrealloc(const uint64_t buffer, const uint32_t size) {
    assert_msg(physalloc_is_initialized,
               "mm: physrealloc() called before physalloc_init()");

    // Allow buffer=0 and buffer=INVALID_PHYS to call physalloc().
    if (__builtin_expect(buffer == 0 || buffer == INVALID_PHYS, 0)) {
        if (__builtin_expect(size == 0, 0)) {
            return INVALID_PHYS;
        }

        return physalloc(size);
    }

    if (__builtin_expect(size == 0, 0)) {
        physalloc_free(buffer);
        printk(LOGLEVEL_WARN,
               "mm: physrealloc(): got size=0, use physalloc_free() instead\n");

        return INVALID_PHYS;
    }

    const uint64_t buffer_size = slab_object_size(phys_to_virt(buffer));
    if (size <= buffer_size) {
        return buffer;
    }

    const uint64_t ret = physalloc(size);
    if (__builtin_expect(ret == INVALID_PHYS, 0)) {
        return INVALID_PHYS;
    }

    memcpy(phys_to_virt(ret), phys_to_virt(buffer), buffer_size);
    physalloc_free(buffer);

    return ret;
}

__debug_optimize(3) void physalloc_free(const uint64_t phys) {
    slab_free(phys_to_virt(phys));
}