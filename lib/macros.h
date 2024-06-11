/*
 * lib/macros.h
 * © suhas pai
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

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
    #if __has_attribute(optimize) && defined(DEBUG) && !defined(RELEASE)
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
#define sizeof_bits_field(type, field) bytes_to_bits(sizeof_field(type, field))

#define LEN_OF(str) (sizeof(str) - 1)

#define __VAR_CONCAT_IMPL(a, b) a##b
#define VAR_CONCAT(a, b) __VAR_CONCAT_IMPL(a, b)

#define __VAR_CONCAT_3_IMPL(a, b, c) a##b##c
#define VAR_CONCAT_3(a, b, c) __VAR_CONCAT_3_IMPL(a, b, c)

#define __TO_STRING_IMPL(x) #x
#define TO_STRING(x) __TO_STRING_IMPL(x)

#define SECTOR_SIZE 512

#define container_of(ptr, type, name) \
    ((type *)(uint64_t)((const void *)ptr - offsetof(type, name)))

#define h_var(token) VAR_CONCAT(VAR_CONCAT_3(__, token, __), __LINE__)
#define countof(carr) (sizeof(carr) / sizeof((carr)[0]))
#define carr_foreach(arr, name) \
    const typeof(&(arr)[0]) h_var(end) = ((arr) + countof(arr)); \
    for (typeof(&(arr)[0]) name = &arr[0]; name != h_var(end); name++)

#define carr_foreach_mut(arr, name) \
    typeof(&(arr)[0]) h_var(end) = ((arr) + countof(arr)); \
    for (typeof(&(arr)[0]) name = &arr[0]; name != h_var(end); name++)

#define swap(a, b) ({ \
    const __auto_type __swaptmp = (b); \
    b = a; \
    a = __swaptmp; \
})

#define max(a, b) ({ \
    const __auto_type __maxa = (a); \
    const __auto_type __maxb = (b); \
    __maxa > __maxb ? __maxa : __maxb; \
})

#define min(a, b) ({ \
    const __auto_type __mina = (a); \
    const __auto_type __minb = (b); \
    __mina < __minb ? __mina : __minb; \
})

#define twovar_cmp(a, b) ({ \
    const __auto_type __twovarcmpa = (a); \
    const __auto_type __twovarcmpb = (b); \
    __twovarcmpa < __twovarcmpb ? -1 \
        : __twovarcmpa > __twovarcmpb ? 1 : 0; \
})

#define reg_to_ptr(type, base, reg) ((type *)((uint64_t)(base) + (reg)))
#define field_to_ptr(type, base, field) \
    reg_to_ptr(type, base, offsetof(type, field))
#define get_to_within_size(x, size) ((x) % (size))

#define distance(begin, end) ((uint64_t)(end) - (uint64_t)(begin))
#define distance_incl(begin, end) (distance(begin, end) + 1)

#define RAND_VAR_NAME() VAR_CONCAT(__random__, __LINE__)
#define bits_to_bytes_roundup(bits) ({ \
        const __auto_type __bitstobytesbits__ = (bits); \
        __bitstobytesbits__ % sizeof_bits(uint8_t) ? \
            bits_to_bytes_noround(__bitstobytesbits__) + 1 : \
            bits_to_bytes_noround(__bitstobytesbits__); \
    })

#define bits_to_bytes_noround(bits) ((bits) / 8)
#define bytes_to_bits(bits) ((bits) * 8)

#define sizeof_bits(n) bytes_to_bits(sizeof(n))
#define mask_for_n_bits(n) \
    ((n == sizeof_bits(uint64_t)) ? \
        ~0ull : ~0ull >> (sizeof_bits(uint64_t) - (n)))

#define has_mask(num, mask) ({ \
    __auto_type __hasmaskmask__ = (mask); \
    ((num) & __hasmaskmask__) == __hasmaskmask__; \
})

#define rm_mask(num, mask) ((num) & ((typeof(num))~(mask)))
#define set_bits_for_mask(ptr, mask, value) ({ \
    __auto_type __setbitsptr__ = (ptr);  \
    if (value) {                         \
        *__setbitsptr__ |= (mask);       \
    } else {                             \
        *__setbitsptr__ = rm_mask(*__setbitsptr__, (mask)); \
    } \
})

#define div_round_up(a, b) ({\
    const __auto_type __divrounda = (a); \
    const __auto_type __divroundb = (b); \
    __divrounda % __divroundb != 0 ? \
        (__divrounda / __divroundb) + 1 : (__divrounda / __divroundb); \
})

#define sign_extend_from_index(num, index) ({ \
    const __auto_type __signextendnum__ = (num); \
    const __auto_type __signextendindex__ = (index); \
    \
    __auto_type __signextendresult__ = __signextendnum__; \
    __auto_type __signextendmask__ = \
        mask_for_n_bits(sizeof_bits(__signextendnum__) - __signextendindex__) \
            << __signextendindex__; \
    \
    set_bits_for_mask(&__signextendresult__, \
                      __signextendmask__, \
                      __signextendnum__ & 1ull << __signextendindex__); \
    __signextendresult__; \
})

#define carr_end(arr) ((arr) + countof(arr))
