/*
 * lib/ctype.c
 * © suhas pai
 */

#include <stdbool.h>
#include <stdint.h>

#include "ctype.h"
#include "macros.h"

enum desc {
    ALPHA_NUM = 1 << 0,

    ALPHA_LOWER = 1 << 1,
    ALPHA_UPPER = 1 << 2,

    ALPHA = 1 << 3,
    CNTRL = 1 << 4,
    DIGIT = 1 << 5,
    GRAPH = 1 << 6,
    PRINT = 1 << 7,
    PUNCT = 1 << 8,
    SPACE = 1 << 9,

    HEX_DIGIT = 1 << 10
};

static uint16_t ctype_array[128] = {
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    PRINT | SPACE,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    CNTRL
};

__optimize(3) static inline bool is_ascii(const C_TYPE c) {
#if defined(BUILD_KERNEL)
    (void)c;
    return true;
#else
    return (c > 0 && c < 128);
#endif
}

__optimize(3) int isalnum(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & ALPHA_NUM) ? 1 : 0;
}

__optimize(3) int isalpha(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & ALPHA) ? 1 : 0;
}

__optimize(3) int iscntrl(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & CNTRL) ? 1 : 0;
}

__optimize(3) int isdigit(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & DIGIT) ? 1 : 0;
}

__optimize(3) int isgraph(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & GRAPH) ? 1 : 0;
}

__optimize(3) int islower(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & ALPHA_LOWER) ? 1 : 0;
}

__optimize(3) int isprint(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & PRINT) ? 1 : 0;
}

__optimize(3) int ispunct(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & PUNCT) ? 1 : 0;
}

__optimize(3) int isspace(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & SPACE) ? 1 : 0;
}

__optimize(3) int isupper(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & ALPHA_UPPER) ? 1 : 0;
}

__optimize(3) int isxdigit(const C_TYPE c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[(int)c] & HEX_DIGIT) ? 1 : 0;
}

__optimize(3) int tolower(const C_TYPE c) {
    if (!isupper(c)) {
        return c;
    }

    // return (c - 'A') + 'a'
    return c + 32;
}

__optimize(3) int toupper(const C_TYPE c) {
    if (!islower(c)) {
        return c;
    }

    // return (c - 'a') + 'A'
    return c - 32;
}