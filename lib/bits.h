/*
 * lib/bits.h
 * © suhas pai
 */

#pragma once

#include "adt/range.h"
#include "macros.h"

#define count_lsb_zero_bits(bad_number, start_index) ({           \
    const typeof(bad_number) __number__ =                         \
        (bad_number) >> (start_index);                            \
    uint8_t __result__ = 0;                                       \
    if (__number__ == 0) {                                        \
        __result__ = (sizeof_bits(__number__) - (start_index));   \
    } else {                                                      \
        __result__ =                                              \
            sizeof(__number__) >= 8 ?                             \
                (typeof(__number__))__builtin_ctzll(__number__) : \
                (typeof(__number__))__builtin_ctz(__number__);    \
    }                                                             \
    __result__;                                                   \
})

#define count_lsb_one_bits(number, start_index) \
    count_lsb_zero_bits((typeof(number))~number, start_index)

#define count_msb_zero_bits(bad_number, start_index) ({\
    const typeof(bad_number) __number__ = (bad_number) << (start_index); \
    uint8_t __result__ = 0;                                       \
    if (__number__ == 0) {                                        \
        __result__ = (sizeof_bits(__number__) - (start_index));   \
    } else {                                                      \
        __result__ =                                              \
            sizeof(__number__) >= 8 ?                             \
                (typeof(__number__))__builtin_clzll(__number__) : \
                (typeof(__number__))__builtin_clz(__number__);    \
    }                                                             \
    __result__;                                                   \
})

#define count_msb_one_bits(number, start_index) \
    count_msb_zero_bits((typeof(number))~number, start_index)

#define FIND_BIT_INVALID UINT64_MAX

uint8_t find_lsb_one_bit(uint64_t number, uint64_t start_index);
uint8_t find_lsb_zero_bit(uint64_t number, uint64_t start_index);

uint8_t find_msb_one_bit(uint64_t number, uint64_t start_index);
uint8_t find_msb_zero_bit(uint64_t number, uint64_t start_index);

struct range
get_range_of_lsb_one_bits(uint64_t number,
                          uint64_t start_index,
                          uint64_t end_index);

struct range
get_range_of_lsb_zero_bits(uint64_t number,
                           uint64_t start_index,
                           uint64_t end_index);

struct range
get_next_range_of_lsb_one_bits(uint64_t number,
                               struct range prev,
                               uint64_t end_index);

struct range
get_next_range_of_lsb_zero_bits(uint64_t number,
                                struct range prev,
                                uint64_t end_index);

#define for_every_lsb_zero_bit(number, start_index, iter_name)                 \
    for (uint64_t iter_name = find_lsb_zero_bit(number, start_index);          \
         iter_name != FIND_BIT_INVALID;                                        \
         iter_name = find_lsb_zero_bit(number, iter_name + 1))

#define for_every_lsb_one_bit(number, start_index, iter_name)                  \
    for (uint64_t iter_name = find_lsb_one_bit(number, start_index);           \
         iter_name != FIND_BIT_INVALID;                                        \
         iter_name = find_lsb_one_bit(number, iter_name + 1))

#define for_every_lsb_zero_bit_rng(number, start_index, end_index, iter_name)  \
    for (struct range iter_name =                                              \
             get_range_of_lsb_zero_bits(number, start_index, end_index);       \
         !range_empty(iter_name);                                              \
         iter_name =                                                           \
            get_next_range_of_lsb_zero_bits(number, iter_name, end_index))

#define for_every_lsb_one_bit_rng(number, start_index, end_index, iter_name)   \
    for (struct range iter_name =                                              \
             get_range_of_lsb_one_bits(number, start_index, end_index);        \
         !range_empty(iter_name);                                              \
         iter_name =                                                           \
            get_next_range_of_lsb_one_bits(number, iter_name, end_index))
