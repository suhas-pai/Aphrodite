/*
 * lib/endian.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

typedef uint16_t le16_t;
typedef uint32_t le32_t;
typedef uint64_t le64_t;

typedef uint16_t be16_t;
typedef uint32_t be32_t;
typedef uint64_t be64_t;

__debug_optimize(3) static inline uint16_t swap_endian_16(const uint16_t num) {
    return (num << 8) | (num >> 8);
}

__debug_optimize(3) static inline uint32_t swap_endian_32(const uint32_t num) {
    return ((num & 0xFF) << 24)
         | ((num & 0xFF00) << 8)
         | ((num & 0xFF0000) >> 8)
         | ((num & 0xFF000000) >> 24);
}

__debug_optimize(3) static inline be64_t swap_endian_64(const uint64_t num) {
    return ((num & 0xFFULL) << 56)
         | ((num & 0xFF00ULL) << 40)
         | ((num & 0xFF0000ULL) << 24)
         | ((num & 0xFF000000ULL) << 8)
         | ((num & 0xFF00000000ULL) >> 8)
         | ((num & 0xFF0000000000ULL) >> 24)
         | ((num & 0xFF000000000000ULL) >> 40)
         | ((num & 0xFF00000000000000ULL) >> 56);
}

__debug_optimize(3) static inline be16_t le16_to_be(const le16_t le) {
    return swap_endian_16(le);
}

__debug_optimize(3) static inline be32_t le32_to_be(const le32_t le) {
    return swap_endian_32(le);
}

__debug_optimize(3) static inline be64_t le64_to_be(const le64_t le) {
    return swap_endian_64(le);
}

__debug_optimize(3) static inline le16_t be16_to_le(const be16_t be) {
    return swap_endian_16(be);
}

__debug_optimize(3) static inline le32_t be32_to_le(const be32_t be) {
    return swap_endian_32(be);
}

__debug_optimize(3) static inline le64_t be64_to_le(const be64_t be) {
    return swap_endian_64(be);
}

#define le_to_be(num) \
    _Generic((num), \
        uint16_t: le16_to_be(num), \
        uint32_t: le32_to_be(num), \
        uint64_t: le64_to_be(num))

#define be_to_le(num) \
    _Generic((num), \
        uint16_t: be16_to_le(num), \
        uint32_t: be32_to_le(num), \
        uint64_t: be64_to_le(num))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    __debug_optimize(3) static inline uint16_t le16_to_cpu(const le16_t le) {
        return le;
    }

    __debug_optimize(3) static inline uint32_t le32_to_cpu(const le32_t le) {
        return le;
    }

    __debug_optimize(3) static inline uint64_t le64_to_cpu(const le64_t le) {
        return le;
    }

    __debug_optimize(3) static inline uint16_t be16_to_cpu(const be16_t be) {
        return be16_to_le(be);
    }

    __debug_optimize(3) static inline uint32_t be32_to_cpu(const be32_t be) {
        return be32_to_le(be);
    }

    __debug_optimize(3) static inline uint64_t be64_to_cpu(const be64_t be) {
        return be64_to_le(be);
    }

    __debug_optimize(3) static inline le16_t cpu16_to_le(const uint16_t num) {
        return num;
    }

    __debug_optimize(3) static inline le32_t cpu32_to_le(const uint32_t num) {
        return num;
    }

    __debug_optimize(3) static inline le64_t cpu64_to_le(const uint64_t num) {
        return num;
    }

    __debug_optimize(3) static inline be16_t cpu16_to_be(const uint16_t num) {
        return be16_to_le(num);
    }

    __debug_optimize(3) static inline be32_t cpu32_to_be(const uint32_t num) {
        return be32_to_le(num);
    }

    __debug_optimize(3) static inline be64_t cpu64_to_be(const uint64_t num) {
        return be64_to_le(num);
    }
#else
    __debug_optimize(3) static inline uint16_t be16_to_cpu(const be16_t be) {
        return be;
    }

    __debug_optimize(3) static inline uint32_t be32_to_cpu(const be32_t be) {
        return be;
    }

    __debug_optimize(3) static inline uint64_t be64_to_cpu(const be64_t be) {
        return be;
    }

    __debug_optimize(3) static inline uint16_t le16_to_cpu(const le16_t le) {
        return le16_to_be(le);
    }

    __debug_optimize(3) static inline uint32_t le32_to_cpu(const le32_t le) {
        return le32_to_be(le);
    }

    __debug_optimize(3) static inline uint64_t le64_to_cpu(const le64_t le) {
        return le64_to_be(le);
    }

    __debug_optimize(3) static inline le16_t cpu16_to_le(const uint16_t num) {
        return be16_to_le(num);
    }

    __debug_optimize(3) static inline le32_t cpu32_to_le(const uint32_t num) {
        return be32_to_le(num);
    }

    __debug_optimize(3) static inline le64_t cpu64_to_le(const uint64_t num) {
        return be64_to_le(num);
    }

    __debug_optimize(3) static inline be16_t cpu16_to_be(const uint16_t num) {
        return num;
    }

    __debug_optimize(3) static inline be32_t cpu32_to_be(const uint32_t num) {
        return num;
    }

    __debug_optimize(3) static inline be64_t cpu64_to_be(const uint64_t num) {
        return num;
    }
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

#define le_to_cpu(num) \
    _Generic((num), \
        uint8_t: (num), \
        le16_t: le16_to_cpu(num), \
        le32_t: le32_to_cpu(num), \
        le64_t: le64_to_cpu(num))

#define be_to_cpu(num) \
    _Generic((num), \
        uint8_t: (num), \
        be16_t: be16_to_cpu(num), \
        be32_t: be32_to_cpu(num), \
        be64_t: be64_to_cpu(num))

#define cpu_to_le(num) \
    _Generic((num), \
        uint8_t: (num), \
        uint16_t: cpu16_to_le(num), \
        uint32_t: cpu32_to_le(num), \
        uint64_t: cpu64_to_le(num))

#define cpu_to_be(num) \
    _Generic((num), \
        uint8_t: (num), \
        uint16_t: cpu16_to_le(num), \
        uint32_t: cpu32_to_le(num), \
        uint64_t: cpu64_to_le(num))
