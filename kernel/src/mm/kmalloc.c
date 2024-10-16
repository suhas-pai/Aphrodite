/*
 * kernel/src/mm/kmalloc.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "kmalloc.h"
#include "slab.h"

static struct slab_allocator kmalloc_slabs[19] = {0};
static bool kmalloc_is_initialized = false;

__debug_optimize(3) bool kmalloc_initialized() {
    return kmalloc_is_initialized;
}

#define SLAB_ALLOC_INIT(size, alloc_flags, flags) \
    assert( \
        slab_allocator_init(&kmalloc_slabs[index], size, alloc_flags, flags)); \
    index++;

void kmalloc_check_slabs() {
#if defined(CHECK_SLABS)
    carr_foreach(kmalloc_slabs, alloc) {
        slab_verify(alloc);
    }
#endif
}

void kmalloc_init() {
    uint8_t index = 0;

    SLAB_ALLOC_INIT(16, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(32, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(64, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(96, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(128, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(192, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(256, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(384, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(512, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(768, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(1024, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(1536, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(2048, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(4096, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(6102, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(8192, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(12288, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(16384, /*alloc_flags=*/0, /*flags=*/0);
    SLAB_ALLOC_INIT(32748, /*alloc_flags=*/0, /*flags=*/0);

    kmalloc_is_initialized = true;
}

__debug_optimize(3) __malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc(const uint32_t size) {
    assert_msg(kmalloc_initialized(),
               "mm: kmalloc() called before kmalloc_init()");

    kmalloc_check_slabs();
    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: kmalloc() got size=0\n");
        return NULL;
    }

    if (__builtin_expect(size > KMALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: kmalloc() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               KMALLOC_MAX);
        return NULL;
    }

    struct slab_allocator *allocator = &kmalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    return slab_alloc(allocator);
}

__debug_optimize(3) __malloclike __malloc_dealloc(kfree, 1)
void *kmalloc_size(const uint32_t size, uint32_t *const size_out) {
    kmalloc_check_slabs();
    assert_msg(kmalloc_initialized(),
               "mm: kmalloc_size() called before kmalloc_init()");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: kmalloc_size() got size=0\n");
        return NULL;
    }

    if (__builtin_expect(size > KMALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: kmalloc_size() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               KMALLOC_MAX);
        return NULL;
    }

    struct slab_allocator *allocator = &kmalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    void *const result = slab_alloc(allocator);
    kmalloc_check_slabs();

    if (__builtin_expect(result != NULL, 1)) {
        *size_out = allocator->object_size;
        return result;
    }

    return NULL;
}

__debug_optimize(3) void *krealloc(void *const buffer, const uint32_t size) {
    assert_msg(kmalloc_is_initialized,
               "mm: krealloc() called before kmalloc_init()");

    // Allow buffer=NULL to call kmalloc().
    if (__builtin_expect(buffer == NULL, 0)) {
        if (__builtin_expect(size == 0, 0)) {
            return NULL;
        }

        return kmalloc(size);
    }

    if (__builtin_expect(size == 0, 0)) {
        kfree(buffer);
        printk(LOGLEVEL_WARN,
               "mm: krealloc(): got size=0, use kfree() instead\n");

        return NULL;
    }

    const uint64_t buffer_size = slab_object_size(buffer);
    if (size <= buffer_size) {
        return buffer;
    }

    void *const ret = kmalloc(size);
    if (__builtin_expect(ret == NULL, 0)) {
        return NULL;
    }

    memcpy(ret, buffer, buffer_size);
    kfree(buffer);

    return ret;
}

__debug_optimize(3) void kfree(void *const buffer) {
    kmalloc_check_slabs();
    assert_msg(kmalloc_is_initialized,
               "mm: kfree() called before kmalloc_init()");

    slab_free(buffer);
}