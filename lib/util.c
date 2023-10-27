/*
 * lib/util.c
 * © suhas pai
 */

#include "macros.h"
#include "util.h"

__optimize(3)
bool index_in_bounds(const uint64_t index, const uint64_t bounds) {
    return index < bounds;
}

__optimize(3)
bool ordinal_in_bounds(const uint64_t ordinal, const uint64_t bounds) {
    return ordinal != 0 && ordinal <= bounds;
}

__optimize(3)
bool index_range_in_bounds(const struct range range, const uint64_t bounds) {
    uint64_t end = 0;
    if (!range_get_end(range, &end)) {
        return false;
    }

    return ordinal_in_bounds(end, bounds);
}

__optimize(3) const char *get_alphanumeric_upper_string() {
    return "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
}

__optimize(3) const char *get_alphanumeric_lower_string() {
    return "0123456789abcdefghijklmnopqrstuvwxyz";
}