/*
 * lib/string.c
 * © suhas pai
 */

#include <stdint.h>

#include "macros.h"
#include "string.h"

#if !defined(BUILD_TEST) && !defined(BUILD_KERNEL)
__optimize(3) size_t strlen(const char *str) {
    size_t result = 0;

    const char *iter = str;
    for (char ch = *iter; ch != '\0'; ch = *(++iter), result++) {}

    return result;
}

__optimize(3) size_t strnlen(const char *const str, const size_t limit) {
    size_t result = 0;

    const char *iter = str;
    char ch = *iter;

    for (size_t i = 0; i != limit && ch != '\0'; ch = *(++iter), result++) {}
    return result;
}

__optimize(3) int strcmp(const char *const str1, const char *const str2) {
    const char *iter = str1;
    const char *jter = str2;

    char ch = *iter, jch = *jter;
    for (; ch != '\0' && jch != '\0'; ch = *(++iter), jch = *(++jter)) {}

    return ch - jch;
}

__optimize(3)
int strncmp(const char *str1, const char *const str2, const size_t length) {
    const char *iter = str1;
    const char *jter = str2;

    char ch = *iter, jch = *jter;
    for (size_t i = 0; i != length; i++) {
        if (ch == '\0' || jch == '\0') {
            break;
        }
    }

    return ch - jch;
}

__optimize(3) char *strchr(const char *const str, const int ch) {
    c_string_foreach (str, iter) {
        if (*iter == ch) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-qual"
            return (char *)iter;
        #pragma GCC diagnostic pop
        }
    }

    return NULL;
}

#endif /* !defined(BUILD_TEST) */

#if defined(__x86_64__)
    #define REP_MIN 16
#endif /* defined(__x86_64__) */

__optimize(3) void *memset_all_ones(void *dst, unsigned long n) {
    void *ret = dst;

#if defined(__x86_64__)
    if (n >= REP_MIN) {
        asm volatile ("rep stosb"
                      :: "D"(dst), "al"(UINT8_MAX), "c"(n)
                      : "memory");
        return ret;
    }
#elif defined(__aarch64__)
    while (n >= (sizeof(uint64_t) * 2)) {
        asm volatile ("stp %0, %0, [%1]" :: "r"(UINT64_MAX), "r"(dst));

        dst += sizeof(uint64_t) * 2;
        n -= sizeof(uint64_t) * 2;
    }
#endif /* defined(__x86_64__) */

    while (n >= sizeof(uint64_t)) {
        *(uint64_t *)dst = UINT64_MAX;

        dst += sizeof(uint64_t);
        n -= sizeof(uint64_t);
    }

    while (n >= sizeof(uint32_t)) {
        *(uint32_t *)dst = UINT32_MAX;

        dst += sizeof(uint32_t);
        n -= sizeof(uint32_t);
    }

    while (n >= sizeof(uint16_t)) {
        *(uint16_t *)dst = UINT16_MAX;

        dst += sizeof(uint16_t);
        n -= sizeof(uint16_t);
    }

    while (n >= sizeof(uint8_t)) {
        *(uint8_t *)dst = UINT8_MAX;

        dst += sizeof(uint8_t);
        n -= sizeof(uint8_t);
    }

    return ret;
}
