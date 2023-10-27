/*
 * lib/adt/string_view.c
 * © suhas pai
 */

#include "lib/string.h"
#include "string_view.h"

__optimize(3) struct string_view sv_drop_front(const struct string_view sv) {
    if (sv.length != 0) {
        return sv_create_nocheck(sv.begin + 1, sv.length - 1);
    }

    return SV_EMPTY();
}

__optimize(3) char *sv_get_begin_mut(const struct string_view sv) {
    return (char *)(uint64_t)sv.begin;
}

__optimize(3) const char *sv_get_end(const struct string_view sv) {
    return sv.begin + sv.length;
}

__optimize(3)
bool sv_compare_c_str(const struct string_view sv, const char *const c_str) {
    return strncmp(sv.begin, c_str, sv.length);
}

__optimize(3)
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
    if (sv.length == 0) {
        return 0;
    }

    return strncmp(sv.begin, sv2.begin, sv.length);
}