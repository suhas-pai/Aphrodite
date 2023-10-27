/*
 * lib/macros.h
 * © suhas pai
 */

#pragma once
#include <stddef.h>

#if !defined(__unused)
    #define __unused __attribute__((unused))
#endif /* !defined(__unused )*/

#if !defined(__noreturn)
    #define __noreturn __attribute__((noreturn))
#endif /* !defined(__noreturn )*/

#if !defined(__packed)
    #define __packed __attribute__((packed))
#endif /* !defined(__packed) */

#if !defined(__printf_format)
    #define __printf_format(last_arg_idx, list_idx) \
        __attribute__((format(printf, last_arg_idx, list_idx)))
#endif /* !defined(__printf_format) */

#if !defined(__optimize)
    #if __has_attribute(optimize)
        #define __optimize(n) __attribute__((optimize(n)))
    #else
        #define __optimize(n)
    #endif /* __has_attribute(optimize) */
#endif /* !defined(__optimize) */

#if !defined(__aligned)
    #if __has_attribute(aligned)
        #define __aligned(n) __attribute__((aligned(n)))
    #else
        #define __aligned(n)
    #endif /* __has_attribute(aligned) */
#endif /* !defined(__aligned) */

#if !defined(__malloclike)
    #if __has_attribute(malloc)
        #define __malloclike __attribute__((malloc))
    #else
        #define __malloclike
    #endif /* __has_attribute(malloc) */
#endif /* !defined(__malloclike) */

#if !defined(__malloc_dealloc)
    // Clang doesn't support __attribute__((malloc)) with arguments
    #if __has_attribute(malloc) && !defined(__clang__)
        #define __malloc_dealloc(func, arg) __attribute__((malloc(func, arg)))
    #else
        #define __malloc_dealloc(func, arg)
    #endif /* __has_attribute(malloc) */
#endif /* !defined(__malloc_dealloc) */

#if !defined(__alloc_size)
    #if __has_attribute(alloc_size)
        #define __alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
    #else
        #define __alloc_size(...)
    #endif /* __has_attribute(alloc_size) */
#endif /* !defined(__alloc_size) */

#if !defined(__hidden)
    #if __has_attribute(visibility)
        #define __hidden __attribute__((visibility("hidden")))
    #else
        #define __hidden
    #endif /* __has_attribute(visibility) */
#endif /* !defined(__hidden) */

#define typeof_field(type, field) typeof(((type *)0)->field)
#define sizeof_field(type, field) sizeof(((type *)0)->field)

#define LEN_OF(str) (sizeof(str) - 1)

#define __VAR_CONCAT_IMPL(a, b) a##b
#define VAR_CONCAT(a, b) __VAR_CONCAT_IMPL(a, b)

#define __VAR_CONCAT_3_IMPL(a, b, c) a##b##c
#define VAR_CONCAT_3(a, b, c) __VAR_CONCAT_3_IMPL(a, b, c)

#define __TO_STRING_IMPL(x) #x
#define TO_STRING(x) __TO_STRING_IMPL(x)

#define container_of(ptr, type, name) \
    ((type *)(uint64_t)((const void *)ptr - offsetof(type, name)))

#define h_var(token) VAR_CONCAT(VAR_CONCAT_3(__, token, __), __LINE__)
#define countof(carr) (sizeof(carr) / sizeof((carr)[0]))
#define for_each_in_carr(arr, name) \
    for (typeof(&(arr)[0]) name = &arr[0]; \
         name != ((arr) + countof(arr)); \
         name++)

#define swap(a, b) ({ \
    const __auto_type __tmp = b; \
    b = a;                 \
    a = __tmp;             \
})

#define max(a, b) ({ \
    const __auto_type __a = (a); \
    const __auto_type __b = (b); \
    __a > __b ? __a : __b; \
})

#define min(a, b) ({ \
    const __auto_type __a = (a); \
    const __auto_type __b = (b); \
    __a < __b ? __a : __b; \
})

#define reg_to_ptr(type, base, reg) ((type *)((uint64_t)(base) + (reg)))
#define field_to_ptr(type, base, field) \
    reg_to_ptr(type, base, offsetof(type, field))
#define get_to_within_size(x, size) ((x) % (size))

#define distance(begin, end) ((uint64_t)(end) - (uint64_t)(begin))
#define distance_incl(begin, end) (distance(begin, end) + 1)

#define RAND_VAR_NAME() VAR_CONCAT(__random__, __LINE__)
#define bits_to_bytes_roundup(bits) \
    ({ \
        const __auto_type __bits__ = (bits); \
        __bits__ % sizeof_bits(uint8_t) ? \
            bits_to_bytes_noround(__bits__) + 1 : \
            bits_to_bytes_noround(__bits__); \
    })

#define bits_to_bytes_noround(bits) ((bits) / 8)
#define bytes_to_bits(bits) ((bits) * 8)

#define sizeof_bits(n) bytes_to_bits(sizeof(n))
#define mask_for_n_bits(n) \
    ((n == sizeof_bits(uint64_t)) ? \
        ~0ull : \
        ~0ull >> (sizeof_bits(uint64_t) - (n)))

#define set_bits_for_mask(ptr, mask, value) ({ \
    if (value) {                         \
        *(ptr) |= (mask);                \
    } else {                             \
        *(ptr) &= (typeof(mask))~(mask); \
    }                                    \
})

#define div_round_up(a, b) ({\
    const __auto_type __a = (a); \
    const __auto_type __b = (b); \
    __a % __b != 0 ? (__a / __b) + 1 : (__a / __b); \
})

#define carr_end(arr) ((arr) + countof(arr))