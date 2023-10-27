/*
 * lib/memory.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "memory.h"

__optimize(3)
uint16_t *memset16(uint16_t *buf, uint64_t count, const uint16_t c) {
#if defined(__x86_64__)
    if (count > 32) {
        void *ret = buf;
        asm volatile ("cld;\n"
                      "rep stosw"
                      : "+D"(buf), "+c" (count) : "a"(c) : "memory");
        return ret;
    }
#endif /* defined(__x86_64__) */

    const uint16_t *const end = buf + count;
    for (uint16_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }

    return buf;
}

__optimize(3)
uint32_t *memset32(uint32_t *buf, uint64_t count, const uint32_t c) {
    void *const ret = buf;
#if defined(__x86_64__)
    if (count > 32) {
        asm volatile ("cld;\n"
                      "rep stosl"
                      : "+D"(buf), "+c" (count) : "a"(c) : "memory");
        return ret;
    }
#endif /* defined(__aarch64__) */

#if defined(__aarch64__)
    if (count >= 2) {
        do {
            asm volatile ("stp %w0, %w0, [%1]" :: "r"(c), "r"(buf));

            buf += 2;
            count -= 2;

            if (count >= 2) {
                continue;
            }

            if (count != 0) {
                *buf = c;
            }

            break;
        } while (true);

        return ret;
    }

    if (count == 1) {
        *buf = c;
    }
#else
    const uint32_t *const end = buf + count;
    for (uint32_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
#endif /* defined(__aarch64__) */

    return ret;
}

__optimize(3)
uint64_t *memset64(uint64_t *buf, uint64_t count, const uint64_t c) {
    void *ret = buf;
#if defined(__x86_64__)
    if (count > 32) {
        asm volatile ("cld;\n"
                      "rep stosq"
                      : "+D"(buf), "+c" (count) : "a"(c) : "memory");
        return ret;
    }
#endif /* defined(__aarch64__) */

#if defined(__aarch64__)
    if (count >= 2) {
        do {
            asm volatile ("stp %0, %0, [%1]" :: "r"(c), "r"(buf));

            buf += 2;
            count -= 2;

            if (count >= 2) {
                continue;
            }

            if (count != 0) {
                *buf = c;
            }

            break;
        } while (true);

        return ret;
    }

    if (count == 1) {
        *buf = c;
    }
#else
    const uint64_t *const end = buf + count;
    for (uint64_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
#endif /* defined(__aarch64__) */

    return ret;
}

__optimize(3) bool
membuf_8_is_all(uint8_t *const buf, const uint64_t count, const uint8_t c) {
    const uint8_t *const end = buf + count;
    for (uint8_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_16_is_all(uint16_t *const buf, const uint64_t count, const uint16_t c) {
    const uint16_t *const end = buf + count;
    for (uint16_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_32_is_all(uint32_t *const buf, const uint64_t count, const uint32_t c) {
    const uint32_t *const end = buf + count;
    for (uint32_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_64_is_all(uint64_t *const buf, const uint64_t count, const uint64_t c) {
    const uint64_t *const end = buf + count;
    for (uint64_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}