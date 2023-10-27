/*
 * lib/bits.c
 * © suhas pai
 */

#include "bits.h"
#include "util.h"

__optimize(3)
uint8_t find_lsb_one_bit(const uint64_t number, const uint64_t start_index) {
    return count_lsb_zero_bits(number, start_index) + start_index;
}

__optimize(3)
uint8_t find_lsb_zero_bit(const uint64_t number, const uint64_t start_index) {
    // Invert the number so we count the lsb 1's to get the lsb zero index
    return count_lsb_one_bits(number, start_index) + start_index;
}

__optimize(3)
uint8_t find_msb_zero_bit(const uint64_t number, const uint64_t start_index) {
    return count_msb_zero_bits(number, start_index) + start_index;
}

__optimize(3)
uint8_t find_msb_one_bit(const uint64_t number, const uint64_t start_index) {
    return count_msb_one_bits(number, start_index) + start_index;
}

__optimize(3) struct range
get_range_of_lsb_one_bits(const uint64_t number,
                          const uint64_t start_index,
                          const uint64_t end_index)
{
    const uint64_t first_bit_index = find_lsb_one_bit(number, start_index);
    if ((first_bit_index == FIND_BIT_INVALID ||
        !index_in_bounds(first_bit_index, end_index)))
    {
        return RANGE_EMPTY();
    }

    const uint64_t one_count = count_lsb_one_bits(number, first_bit_index);

    struct range result = RANGE_INIT(first_bit_index, one_count);
    uint64_t result_end = 0;

    if (range_get_end(result, &result_end)) {
        if (result_end > end_index) {
            result.size = end_index - result.front;
        }
    } else {
        result.size = end_index - result.front;
    }

    return result;
}

__optimize(3) struct range
get_range_of_lsb_zero_bits(const uint64_t number,
                           const uint64_t start_index,
                           const uint64_t end_index)
{
    /*
     * We can avoid extra checks/branches by bitwise inversing the number and
     * finding one bits instead.
     */

    return get_range_of_lsb_one_bits(~number, start_index, end_index);
}

__optimize(3) struct range
get_next_range_of_lsb_zero_bits(const uint64_t number,
                                const struct range prev,
                                const uint64_t end_index)
{
    const uint64_t prev_end = range_get_end_assert(prev);
    if (prev_end == sizeof_bits(uint64_t)) {
        return RANGE_EMPTY();
    }

    return get_range_of_lsb_zero_bits(number, prev_end, end_index);
}

__optimize(3) struct range
get_next_range_of_lsb_one_bits(const uint64_t number,
                               const struct range prev,
                               const uint64_t end_index)
{
    const uint64_t prev_end = range_get_end_assert(prev);
    if (prev_end == sizeof_bits(uint64_t)) {
        return RANGE_EMPTY();
    }

    return get_range_of_lsb_one_bits(number, prev_end, end_index);
}