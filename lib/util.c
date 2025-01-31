/*
 * lib/util.c
 * © suhas pai
 */

#include "macros.h"
#include "util.h"

__debug_optimize(3)
bool index_in_bounds(const uint64_t index, const uint64_t bounds) {
    return index < bounds;
}

__debug_optimize(3)
bool ordinal_in_bounds(const uint64_t ordinal, const uint64_t bounds) {
    return ordinal != 0 && ordinal <= bounds;
}

__debug_optimize(3)
bool index_range_in_bounds(const struct range range, const uint64_t bounds) {
    uint64_t end = 0;
    if (__builtin_expect(!range_get_end(range, &end), 0)) {
        return false;
    }

    return end <= bounds;
}

__debug_optimize(3)
uint32_t circular_index_get_next(const uint32_t index, const uint32_t bounds) {
    return (index + 1) % bounds;
}

__debug_optimize(3)
uint32_t circular_index_get_prev(const uint32_t index, const uint32_t bounds) {
    return (index + bounds - 1) % bounds;
}

__debug_optimize(3) const char *get_alphanumeric_upper_string() {
    return "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
}

__debug_optimize(3) const char *get_alphanumeric_lower_string() {
    return "0123456789abcdefghijklmnopqrstuvwxyz";
}
