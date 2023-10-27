/*
 * lib/adt/range.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/math.h"
#include "lib/overflow.h"
#include "lib/util.h"

__optimize(3) struct range range_create_upto(const uint64_t size) {
    return RANGE_INIT(0, size);
}

__optimize(3)
struct range range_create_end(const uint64_t front, const uint64_t end) {
    assert(front <= end);
    return RANGE_INIT(front, (end - front));
}

__optimize(3) bool
range_multiply(const struct range range,
               const uint64_t mult,
               struct range *const result_out)
{
    struct range result = RANGE_EMPTY();
    if (!check_mul(range.front, mult, &result.front) ||
        !check_mul(range.size, mult, &result.size))
    {
        return false;
    }

    *result_out = result;
    return true;
}

__optimize(3)
struct range range_align_in(const struct range range, const uint64_t boundary) {
    uint64_t front = 0;
    if (!align_up(range.front, boundary, &front) ||
        (front - range.front) >= range.size)
    {
        return RANGE_EMPTY();
    }

    return RANGE_INIT(front, align_down(range.size, boundary));
}

__optimize(3)
struct range range_from_index(const struct range range, const uint64_t index) {
    assert(range_has_index(range, index));
    return RANGE_INIT(range.front + index, range.size - index);
}

__optimize(3)
struct range range_from_loc(const struct range range, const uint64_t loc) {
    assert(range_has_loc(range, loc));
    return RANGE_INIT(loc, range.size - (loc - range.front));
}

__optimize(3) bool
range_align_out(const struct range range,
                const uint64_t boundary,
                struct range *const result_out)
{
    uint64_t size = 0;
    if (!align_up(range.size, boundary, &size)) {
        return false;
    }

    *result_out = RANGE_INIT(align_down(range.front, boundary), size);
    return true;
}

__optimize(3) bool
range_round_up(const struct range range,
               const uint64_t mult,
               struct range *const result_out)
{
    uint64_t front = 0;
    uint64_t size = 0;

    if (!round_up(range.front, mult, &front)) {
        return false;
    }

    if (!round_up(range.size, mult, &size)) {
        return false;
    }

    result_out->front = front;
    result_out->size = size;

    return true;
}

__optimize(3) bool
range_round_up_subrange(const struct range range,
                        const uint64_t mult,
                        struct range *const result_out)
{
    struct range result = RANGE_EMPTY();
    if (!round_up(range.front, mult, &result.front)) {
        return false;
    }

    if (result.front - range.front >= range.size) {
        return false;
    }

    result.size = range.size - (result.front - range.front);
    *result_out = result;

    return true;
}

__optimize(3)
bool range_has_index(const struct range range, const uint64_t index) {
    return index_in_bounds(index, range.size);
}

__optimize(3) bool range_has_loc(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) < range.size);
}

__optimize(3) bool range_has_end(const struct range range, const uint64_t end) {
    return (end > range.front && (end - range.front) <= range.size);
}

__optimize(3)
bool range_get_end(const struct range range, uint64_t *const end_out) {
    return check_add(range.front, range.size, end_out);
}

__optimize(3) bool range_overflows(const struct range range) {
    uint64_t result = 0;
    return !check_add(range.front, range.size, &result);
}

__optimize(3)
bool range_above(const struct range range, const struct range above) {
    return range_is_loc_above(range, above.front);
}

__optimize(3)
bool range_below(const struct range range, const struct range below) {
    return range_is_loc_below(range, below.front);
}

__optimize(3) bool range_empty(const struct range range) {
    return range.size == 0;
}

__optimize(3)
bool range_is_loc_above(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) >= range.size);
}

__optimize(3)
bool range_is_loc_below(const struct range range, const uint64_t loc) {
    return (loc < range.front);
}

__optimize(3)
bool range_has_align(const struct range range, const uint64_t align) {
    return has_align(range.front, align) && has_align(range.size, align);
}

__optimize(3) uint64_t range_get_end_assert(const struct range range) {
    uint64_t result = 0;
    assert(range_get_end(range, &result));

    return result;
}

__optimize(3) struct range
subrange_from_index(const struct range range, const uint64_t index) {
    assert(range_has_index(range, index));
    const struct range result = {
        .front = index,
        .size = range.size - index
    };

    return result;
}

__optimize(3) struct range
subrange_to_full(const struct range range, const struct range index) {
    assert(range_has_index_range(range, index));
    const struct range result = {
        .front = range.front + index.front,
        .size = index.size
    };

    return result;
}

__optimize(3)
uint64_t range_loc_for_index(const struct range range, const uint64_t index) {
    assert(range_has_index(range, index));
    return range.front + index;
}

__optimize(3)
uint64_t range_index_for_loc(const struct range range, const uint64_t loc) {
    assert(range_has_loc(range, loc));
    return loc - range.front;
}

__optimize(3)
bool range_has(const struct range range, const struct range other) {
    if (!range_has_loc(range, other.front)) {
        return false;
    }

    const uint64_t index = range_index_for_loc(range, other.front);
    if (!range_has_index(range, index)) {
        return false;
    }

    return range.size - index >= other.size;
}

__optimize(3)
bool range_has_index_range(struct range range, struct range other) {
    if (!range_has_index(range, other.front)) {
        return false;
    }

    return range.size - other.front >= other.size;
}

__optimize(3)
bool range_overlaps(const struct range range, const struct range other) {
    if (range_has_loc(range, other.front)) {
        return true;
    }

    if (range_has_loc(other, range.front)) {
        return true;
    }

    return false;
}