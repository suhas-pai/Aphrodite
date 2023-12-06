/*
 * lib/alloc.h
 * © suhas pai
 */

#pragma once

#if defined(BUILD_KERNEL)
    #include "kernel/src/mm/kmalloc.h"
    #include "overflow.h"

    #define malloc(size) kmalloc(size)
    #define calloc(amt, size) kmalloc(check_mul_assert((uint32_t)(amt), (size)))
    #define calloc_size(amt, size, out) ({ \
        uint32_t __calloc_new_size__ = 0; \
        uint32_t __calloc_obj_size__ = (size); \
        __auto_type __calloc_result__ = \
            kmalloc_size(check_mul_assert((uint32_t)(amt), \
                                          __calloc_obj_size__), \
                         &__calloc_new_size__); \
        *(out) = __calloc_new_size__ / __calloc_obj_size__; \
        __calloc_result__; \
    })

    #define malloc_size(size, out) kmalloc_size(size, out)
    #define realloc(buffer, size) krealloc(buffer, size)
    #define free(buffer) kfree(buffer)
#elif defined(BUILD_TEST)
    #include <stdlib.h>
    #define malloc_size(size, out) ({ \
        __auto_type __malloc_size__ = (size); \
        *(out) = __malloc_size__; \
        malloc(__malloc_size__); \
    })
    #define calloc_size(amt, size, out) ({ \
        __auto_type __calloc_amt__ = (amt); \
        *(out) = __calloc_amt__; \
        calloc(__calloc_amt__, (size)); \
    })
#else
    void *malloc(uint64_t size);
    void *calloc(uint64_t amount, uint64_t size);
    void *calloc(uint64_t amount, uint64_t size, uint64_t *size_out);
    void *malloc_size(uint64_t size, uint64_t *out);
    void *realloc(void *buffer, uint64_t size);
    void free(void *buffer);
#endif /* defined(BUILD_KERNEL) */
