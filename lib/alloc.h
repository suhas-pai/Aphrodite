/*
 * lib/alloc.h
 * © suhas pai
 */

#pragma once

#if defined(BUILD_KERNEL)
    #include "kernel/src/mm/kmalloc.h"

    #define malloc(size) kmalloc(size)
    #define malloc_size(size, out) kmalloc_size(size, out)
    #define realloc(buffer, size) krealloc(buffer, size)
    #define free(buffer) kfree(buffer)
#elif defined(BUILD_TEST)
    #include <stdlib.h>
    #define malloc_size(size, out) ({ \
        *(out) = (size); \
        malloc(size); \
    })
#else
    void malloc(uint64_t size);
    void malloc_size(uint64_t size, uint64_t *out);
    void realloc(void *buffer, uint64_t size);
    void free(void *buffer);
#endif /* defined(BUILD_KERNEL) */
