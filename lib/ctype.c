/*
 * lib/ctype.c
 * © suhas pai
 */

#include <stdbool.h>

#include "ctype.h"
#include "macros.h"

enum ctype_masks {
    __ALPHA_LOWER = 1 << 0,
    __ALPHA_UPPER = 1 << 1,

    __CNTRL = 1 << 2,
    __DIGIT = 1 << 3,
    __GRAPH = 1 << 4,
    __PRINT = 1 << 5,
    __PUNCT = 1 << 6,
    __SPACE = 1 << 7,

    __HEX_DIGIT = 1 << 8
};

static const uint16_t ctype_array[128] = {
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL | __SPACE,
    __CNTRL | __SPACE,
    __CNTRL | __SPACE,
    __CNTRL | __SPACE,
    __CNTRL | __SPACE,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __CNTRL,
    __PRINT | __SPACE,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __DIGIT | __GRAPH | __PRINT | __HEX_DIGIT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER | __HEX_DIGIT,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __ALPHA_UPPER,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT | __HEX_DIGIT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __ALPHA_LOWER | __PRINT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __GRAPH | __PRINT | __PUNCT,
    __CNTRL
};

__debug_optimize(3) static inline bool is_ascii(const C_TYPE c) {
#if defined(BUILD_KERNEL)
    (void)c;
    return true;
#else
    return c > 0 && c < 128;
#endif
}

__debug_optimize(3) int isalnum(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    const uint16_t mask = __ALPHA_LOWER | __ALPHA_UPPER | __DIGIT;
    return (ctype_array[(int)c] & mask) ? 1 : 0;
}

__debug_optimize(3) int isalpha(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & (__ALPHA_LOWER | __ALPHA_UPPER)) ? 1 : 0;
}

__debug_optimize(3) int iscntrl(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __CNTRL) ? 1 : 0;
}

__debug_optimize(3) int isdigit(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __DIGIT) ? 1 : 0;
}

__debug_optimize(3) int isgraph(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __GRAPH) ? 1 : 0;
}

__debug_optimize(3) int islower(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __ALPHA_LOWER) ? 1 : 0;
}

__debug_optimize(3) int isprint(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __PRINT) ? 1 : 0;
}

__debug_optimize(3) int ispunct(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __PUNCT) ? 1 : 0;
}

__debug_optimize(3) int isspace(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __SPACE) ? 1 : 0;
}

__debug_optimize(3) int isupper(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __ALPHA_UPPER) ? 1 : 0;
}

__debug_optimize(3) int isxdigit(const C_TYPE c) {
    if (__builtin_expect(!is_ascii(c), 0)) {
        return 0;
    }

    return (ctype_array[(int)c] & __HEX_DIGIT) ? 1 : 0;
}

__debug_optimize(3) int tolower(const C_TYPE c) {
    if (!isupper(c)) {
        return c;
    }

    // return (c - 'A') + 'a'
    return c + 32;
}

__debug_optimize(3) int toupper(const C_TYPE c) {
    if (!islower(c)) {
        return c;
    }

    // return (c - 'a') + 'A'
    return c - 32;
}