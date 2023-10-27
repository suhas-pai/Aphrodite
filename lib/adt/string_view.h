/*
 * lib/adt/string_view.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/overflow.h"
#include "lib/string.h"

struct string_view {
    const char *begin;
    uint64_t length;
};

#define SV_EMPTY() \
    ((struct string_view){ \
        .begin = "", \
        .length = 0 \
    })

#define SV_STATIC(c_str) \
    ((struct string_view){ .begin = (c_str), .length = LEN_OF(c_str) })
#define sv_foreach(sv, iter) \
    for (const char *iter = sv.begin; iter != (sv.begin + sv.length); iter++)

#define SV_FMT "%.*s"
#define SV_FMT_ARGS(sv) (int)(sv).length, (sv).begin

__optimize(3)
static inline struct string_view sv_create(const char *const str) {
    return (struct string_view){ .begin = str, .length = strlen(str) };
}

__optimize(3) static inline struct string_view
sv_create_nocheck(const char *const c_str, const uint64_t length) {
    return (struct string_view){
        .begin = c_str,
        .length = length
    };
}

__optimize(3) static inline struct string_view
sv_create_end(const char *const c_str, const char *const end) {
    assert(c_str <= end);
    return sv_create_nocheck(c_str, distance(c_str, end));
}

__optimize(3) static inline struct string_view
sv_create_length(const char *const c_str, const uint64_t length) {
    check_add_assert((uint64_t)c_str, length);
    return sv_create_nocheck(c_str, length);
}

struct string_view sv_drop_front(struct string_view sv);

char *sv_get_begin_mut(struct string_view sv);
const char *sv_get_end(struct string_view sv);

bool sv_compare_c_str(struct string_view sv, const char *c_str);
int sv_compare(struct string_view sv, struct string_view sv2);

__optimize(3) static inline
bool sv_equals_c_str(const struct string_view sv, const char *const c_str) {
    return sv_compare_c_str(sv, c_str) == 0;
}

__optimize(3) static inline
bool sv_equals(const struct string_view sv, const struct string_view sv2) {
    return sv_compare(sv, sv2) == 0;
}