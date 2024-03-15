/*
 * lib/adt/bitset.h
 */

#pragma once
#include "lib/bits.h"

#define BITSET_INVALID UINT64_MAX

#define bitset_decl(name, n) uint64_t name[(n) / sizeof_bits(uint64_t)] = {0}
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

#define bitset_find_set(name, invert) \
    ({ \
        __auto_type h_var(result) = BITSET_INVALID; \
        carr_foreach(name, h_var(iter)) { \
            const uint8_t h_var(bit_index) = \
                find_lsb_one_bit(*h_var(iter), /*start_index=*/0); \
            if (h_var(bit_index) != sizeof_bits(uint64_t)) { \
                if (invert) { \
                    *h_var(iter) = \
                        rm_mask(*h_var(iter), 1ull << h_var(bit_index)); \
                } \
                h_var(result) = \
                    (uint64_t)((h_var(iter) - (name)) + 1) * h_var(bit_index); \
                break; \
            } \
        } \
        h_var(result); \
    })

#define bitset_find_unset(name, invert) \
    ({ \
        __auto_type h_var(result) = BITSET_INVALID; \
        carr_foreach(name, h_var(iter)) { \
            const uint8_t h_var(bit_index) = \
                find_lsb_zero_bit(*h_var(iter), /*start_index=*/0); \
            if (h_var(bit_index) != sizeof_bits(uint64_t)) { \
                if (invert) { \
                    *h_var(iter) |= 1ull << h_var(bit_index); \
                } \
                h_var(result) = \
                    (uint64_t)((h_var(iter) - (name)) + 1) * h_var(bit_index); \
                break; \
            } \
        } \
        h_var(result); \
    })
