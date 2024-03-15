/*
 * lib/adt/bitset.h
 */

#pragma once

#include "lib/bits.h"
#include "lib/util.h"

#define BITSET_INVALID UINT64_MAX

#define bitset_size_for_count(n) \
    div_round_up((uint32_t)(n), sizeof_bits(uint64_t))
#define bitset_decl(name, n) \
    uint64_t name[((n) / sizeof_bits(uint64_t)) + \
                  ((n) % sizeof_bits(uint64_t))] = {0}

#define bitset_has(name, index) \
    (name[(index) / sizeof_bits(uint64_t)] & \
        1ull << ((index) % sizeof_bits(uint64_t)))

#define bitset_set(name, index) \
    name[(index) / sizeof_bits(uint64_t)] |= \
        1ull << ((index) % sizeof_bits(uint64_t))

#define bitset_unset(name, index) \
    name[(index) / sizeof_bits(uint64_t)] = \
        rm_mask(name[(index) / sizeof_bits(uint64_t)], \
                1ull << ((index) % sizeof_bits(uint64_t)))

#define bitset_find_set(bitset, length, invert) \
    ({ \
        __auto_type h_var(result) = BITSET_INVALID; \
        __auto_type h_var(count) = bitset_size_for_count(length); \
        for (uint32_t h_var(index) = 0; \
             h_var(index) != h_var(count); \
             h_var(index)++) \
        { \
            const uint8_t h_var(bit_index) = \
                find_lsb_zero_bit((bitset)[h_var(index)], /*start_index=*/0); \
            if (h_var(bit_index) == sizeof_bits(uint64_t)) { \
                continue; \
            } \
            \
            h_var(result) = (uint64_t)((h_var(index) + 1) * h_var(bit_index)); \
            if (h_var(result) == BITSET_INVALID) { \
                break; \
            } \
            \
            if (index_in_bounds(h_var(result), length)) {\
                if (invert) { \
                    (bitset)[h_var(index)] = \
                        rm_mask((bitset)[h_var(index)], \
                                1ull << h_var(bit_index)); \
                } \
            } else { \
                h_var(result) = BITSET_INVALID; \
            } \
            break; \
        } \
        h_var(result); \
    })

#define bitset_find_unset(bitset, length, invert) \
    ({ \
        __auto_type h_var(result) = BITSET_INVALID; \
        __auto_type h_var(count) = bitset_size_for_count(length); \
        for (uint32_t h_var(index) = 0; \
             h_var(index) != h_var(count); \
             h_var(index)++) \
        { \
            const uint8_t h_var(bit_index) = \
                find_lsb_zero_bit((bitset)[h_var(index)], /*start_index=*/0); \
            if (h_var(bit_index) == sizeof_bits(uint64_t)) { \
                continue; \
            } \
            \
            h_var(result) = (uint64_t)((h_var(index) + 1) * h_var(bit_index)); \
            if (h_var(result) == BITSET_INVALID) { \
                break; \
            } \
            \
            if (index_in_bounds(h_var(result), length)) {\
                if (invert) { \
                    (bitset)[h_var(index)] |= 1ull << h_var(bit_index); \
                } \
            } else { \
                h_var(result) = BITSET_INVALID; \
            } \
            break; \
        } \
        h_var(result); \
    })
