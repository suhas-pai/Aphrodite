/*
 * lib/adt/string_view.c
 * © suhas pai
 */

#include "lib/util.h"
#include "string_view.h"

__debug_optimize(3) struct string_view
sv_substring_length(const struct string_view sv,
                    const uint32_t index,
                    const uint32_t length)
{
    assert(sv_has_index_range(sv, RANGE_INIT(index, length)));
    return sv_create_length(sv.begin + index, length);
}

__debug_optimize(3) struct string_view
sv_substring_from(const struct string_view sv, const uint32_t index) {
    assert(index == sv.length || sv_has_index(sv, index));
    return sv_create_length(sv.begin + index, sv.length - index);
}

__debug_optimize(3) struct string_view
sv_substring_upto(const struct string_view sv, const uint32_t index) {
    assert(index == sv.length || sv_has_index(sv, index));
    return sv_create_length(sv.begin, index);
}

__debug_optimize(3)
struct string_view sv_drop_front(const struct string_view sv) {
    if (sv.length != 0) {
        return sv_create_nocheck(sv.begin + 1, sv.length - 1);
    }

    return SV_EMPTY();
}

__debug_optimize(3) char *sv_begin_mut(const struct string_view sv) {
    return (char *)(uint64_t)sv.begin;
}

__debug_optimize(3) const char *sv_end(const struct string_view sv) {
    return sv.begin + sv.length;
}

__debug_optimize(3)
bool sv_compare_c_str(const struct string_view sv, const char *const c_str) {
    return strncmp(sv.begin, c_str, sv.length);
}

__debug_optimize(3)
int sv_compare(const struct string_view sv, const struct string_view sv2) {
    if (sv.length > sv2.length) {
        if (sv2.length == 0) {
            return *sv.begin;
        }

        return strncmp(sv.begin, sv2.begin, sv2.length);
    } else if (sv.length < sv2.length) {
        if (sv.length == 0) {
            return -*sv2.begin;
        }

        return strncmp(sv.begin, sv2.begin, sv.length);
    }

    // Both svs are empty
    if (__builtin_expect(sv.length == 0, 0)) {
        return 0;
    }

    return strncmp(sv.begin, sv2.begin, sv.length);
}

__debug_optimize(3)
bool sv_has_index(const struct string_view sv, const uint32_t index) {
    return index_in_bounds(index, sv.length);
}

__debug_optimize(3)
bool sv_has_index_range(const struct string_view sv, const struct range range) {
    return index_range_in_bounds(range, sv.length);
}

__debug_optimize(3) char sv_front(const struct string_view sv) {
    assert(sv.length != 0);
    return sv.begin[0];
}

__debug_optimize(3) char sv_back(const struct string_view sv) {
    assert(sv.length != 0);
    return sv.begin[sv.length - 1];
}

__debug_optimize(3)  int64_t
sv_find_char(const struct string_view sv, const uint32_t index, const char ch) {
    assert(sv_has_index(sv, index));

    char *const ptr = strchr(sv.begin + index, ch);
    if (ptr != NULL) {
        return (uint32_t)distance(sv.begin, ptr);
    }

    return -1;
}

__debug_optimize(3)
bool sv_equals(const struct string_view sv, const struct string_view sv2) {
    if (sv.length != sv2.length) {
        return false;
    }

    // Both svs are empty
    if (__builtin_expect(sv.length == 0, 0)) {
        return true;
    }

    return strncmp(sv.begin, sv2.begin, sv.length) == 0;
}

__debug_optimize(3) bool
sv_has_prefix(const struct string_view sv, const struct string_view prefix) {
    if (__builtin_expect(prefix.length > sv.length, 0)) {
        return false;
    }

    return strncmp(sv.begin, prefix.begin, prefix.length) == 0;
}

__debug_optimize(3) bool
sv_has_suffix(const struct string_view sv, const struct string_view suffix) {
    if (__builtin_expect(suffix.length > sv.length, 0)) {
        return false;
    }

    const uint32_t index = sv.length - suffix.length;
    return strncmp(sv.begin + index, suffix.begin, suffix.length) == 0;
}