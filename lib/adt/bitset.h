/*
 * lib/adt/bitset.h
 * © suhas pai
 */

#pragma once

#include <lib/bits.h>
#include <lib/util.h>

#define BITSET_INVALID UINT64_MAX

#define bitset_size_for_count(n) \
    div_round_up((uint32_t)(n), sizeof_bits(uint64_t))
#define bitset_decl(name, n) \
    uint64_t name[((n) / sizeof_bits(uint64_t)) + \
                  ((n) % sizeof_bits(uint64_t))] = {0}

#define bitset_has(name, index) \
    ((name)[(index) / sizeof_bits(uint64_t)] & \
        1ull << ((index) % sizeof_bits(uint64_t)))

#define bitset_set(name, index) \
    (name)[(index) / sizeof_bits(uint64_t)] |= \
        1ull << ((index) % sizeof_bits(uint64_t))

#define bitset_unset(name, index) \
    (name)[(index) / sizeof_bits(uint64_t)] = \
        rm_mask((name)[(index) / sizeof_bits(uint64_t)], \
                1ull << ((index) % sizeof_bits(uint64_t)))

uint64_t bitset_find_set(uint64_t *bitset, uint64_t length, bool invert);
uint64_t bitset_find_unset(uint64_t *bitset, uint64_t length, bool invert);
