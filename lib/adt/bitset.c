/*
 * lib/adt/bitset.c
 * © suhas pai
 */

#include "bitset.h"

__debug_optimize(3) uint64_t
bitset_find_set(uint64_t *const bitset,
                const uint64_t length,
                const bool invert)
{
    uint64_t result = BITSET_INVALID;
    const uint64_t count = bitset_size_for_count(length);

    for (uint32_t index = 0; index != count; index++) {
        const uint8_t bit_index =
            find_lsb_zero_bit(bitset[index], /*start_index=*/0);

        if (bit_index == sizeof_bits(uint64_t)) {
            continue;
        }

        result = (uint64_t)((index + 1) * bit_index);
        if (result == BITSET_INVALID) { \
            break;
        }

        if (index_in_bounds(result, length)) {
            if (invert) {
                bitset[index] = rm_mask((bitset)[index], 1ull << bit_index);
            }
        } else {
            result = BITSET_INVALID;
        }

        break;
    }

    return result;
}

__debug_optimize(3) uint64_t
bitset_find_unset(uint64_t *const bitset,
                  const uint64_t length,
                  const bool invert)
{
    uint64_t result = BITSET_INVALID;
    const uint64_t count = bitset_size_for_count(length);

    for (uint32_t index = 0; index != count; index++) {
        const uint8_t bit_index =
            find_lsb_zero_bit(bitset[index], /*start_index=*/0);

        if (bit_index == sizeof_bits(uint64_t)) {
            continue;
        }

        result = (uint64_t)((index + 1) * bit_index);
        if (result == BITSET_INVALID) {
            break;
        }

        if (index_in_bounds(result, length)) {
            if (invert) {
                bitset[index] |= 1ull << bit_index;
            }
        } else {
            result = BITSET_INVALID;
        }

        break;
    }

    return result;
}
