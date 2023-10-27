/*
 * lib/adt/bitmap.c
 * © suhas pai
 */

#include "lib/math.h"
#include "lib/memory.h"
#include "lib/util.h"

#include "bitmap.h"

struct bitmap bitmap_alloc(const uint64_t bit_count) {
    return (struct bitmap){
        .gbuffer = gbuffer_alloc(bits_to_bytes_roundup(bit_count)),
    };
}

struct bitmap bitmap_open(void *const buffer, const uint64_t byte_count) {
    return (struct bitmap){
        .gbuffer =
            gbuffer_open(buffer,
                         /*used=*/byte_count,
                         /*capacity=*/byte_count,
                         /*is_alloc=*/false),
    };
}

__optimize(3) uint64_t bitmap_capacity(const struct bitmap *const bitmap) {
    return bytes_to_bits(gbuffer_capacity(bitmap->gbuffer));
}

static inline uint64_t
find_multiple_unset(struct bitmap *const bitmap,
                    uint64_t count,
                    uint64_t start_index,
                    const bool set)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = (start_index % sizeof_bits(uint8_t));

    /*
     * Loop over every word, and do the following:
     *   (a) Find every distinct sequence of zeros, and return if the sequence
     *       is large enough
     *   (b) If a sequence is not large enough, and is at the msb end of the
     *       word, check the leading zeros of the next word, and return if the
     *       combined sequence is large enough.
     */

    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

    uint64_t current_range_start_index = 0;
    uint64_t current_range_zero_count = 0;

#define LOOP_OVER_RANGES_FOR_TYPE(type)                                        \
    for (uint64_t i = 0;                                                       \
         distance(ptr, end) >= sizeof(type);                                   \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type),           \
         start_index = 0, i++)                                                 \
    {                                                                          \
        const type word = *(type *)ptr;                                        \
        if (current_range_zero_count != 0) {                                   \
            const uint8_t lsb_zero_count =                                     \
                min(sizeof_bits(type),                                         \
                    count_lsb_one_bits(word, /*start_index=*/0));              \
                                                                               \
            if (current_range_zero_count + lsb_zero_count >= count) {          \
                if (set) {                                                     \
                    bitmap_set_range(bitmap,                                   \
                                     RANGE_INIT(start_index, count),           \
                                     /*value=*/true);                          \
                }                                                              \
                                                                               \
                return current_range_start_index;                              \
            }                                                                  \
                                                                               \
            if (lsb_zero_count == sizeof_bits(type)) {                         \
                current_range_zero_count += lsb_zero_count;                    \
                continue;                                                      \
            }                                                                  \
                                                                               \
            /*
             * If the lsb sequence of zeroes plus the msb zeros of the last word
             * wasn't long enough, the lsb sequence isn't long enough by itself.
             *
             * Avoid re-finding this same sequence of lsb zeros by pointing
             * start_index to the sequence's end.
             */                                                                \
                                                                               \
            start_index = lsb_zero_count;                                      \
            current_range_start_index = 0;                                     \
            current_range_zero_count = 0;                                      \
        }                                                                      \
                                                                               \
        for_every_lsb_zero_bit_rng(word, start_index, sizeof_bits(type), iter) \
        {                                                                      \
            if (iter.size < count) {                                           \
                /*
                 * If the range isn't long enough, but goes to the end of the
                 * word, then the next word can have the remaining zeroes needed
                 * in its lsb.
                 */                                                            \
                                                                               \
                if (range_get_end_assert(iter) == sizeof_bits(type)) {         \
                    current_range_start_index = bit_index_of_ptr + iter.front; \
                    current_range_zero_count = iter.size;                      \
                                                                               \
                    break;                                                     \
                }                                                              \
                                                                               \
                continue;                                                      \
            }                                                                  \
                                                                               \
            if (set) {                                                         \
                bitmap_set_range(bitmap,                                       \
                                 RANGE_INIT(start_index, count),               \
                                 /*value=*/true);                              \
            }                                                                  \
                                                                               \
            return bit_index_of_ptr + iter.front;                              \
        }                                                                      \
                                                                               \
        start_index = 0;                                                       \
    }

    LOOP_OVER_RANGES_FOR_TYPE(uint64_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint32_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint16_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint8_t);
#undef LOOP_OVER_RANGES_FOR_TYPE

    return true;
}

static inline uint64_t
find_multiple_set(struct bitmap *const bitmap,
                  uint64_t count,
                  uint64_t start_index,
                  const bool unset)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = (start_index % sizeof_bits(uint8_t));

    /*
     * Loop over every word, and do the following:
     *   (a) Find every distinct sequence of zeros, and return if the sequence
     *       is large enough
     *   (b) If a sequence is not large enough, and is at the msb end of the
     *       word, check the leading zeros of the next word, and return if the
     *       combined sequence is large enough.
     */

    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

    uint64_t current_range_start_index = 0;
    uint64_t current_range_zero_count = 0;

#define LOOP_OVER_RANGES_FOR_TYPE(type)                                        \
    for (uint64_t i = 0;                                                       \
         distance(ptr, end) >= sizeof(type);                                   \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type),           \
         start_index = 0, i++)                                                 \
    {                                                                          \
        const type word = *(type *)ptr;                                        \
        if (current_range_zero_count != 0) {                                   \
            const uint8_t lsb_zero_count =                                     \
                min(sizeof_bits(type),                                         \
                    count_lsb_zero_bits(word, /*start_index=*/0));             \
                                                                               \
            if (current_range_zero_count + lsb_zero_count >= count) {          \
                if (unset) {                                                   \
                    bitmap_set_range(bitmap,                                   \
                                     RANGE_INIT(start_index, count),           \
                                     /*value=*/false);                         \
                }                                                              \
                                                                               \
                return current_range_start_index;                              \
            }                                                                  \
                                                                               \
            if (lsb_zero_count == sizeof_bits(type)) {                         \
                current_range_zero_count += lsb_zero_count;                    \
                continue;                                                      \
            }                                                                  \
                                                                               \
            /*
             * If the lsb sequence of zeroes plus the msb zeros of the last word
             * wasn't long enough, the lsb sequence isn't long enough by itself.
             *
             * Avoid re-finding this same sequence of lsb zeros by pointing
             * start_index to the sequence's end.
             */                                                                \
                                                                               \
            start_index = lsb_zero_count;                                      \
            current_range_start_index = 0;                                     \
            current_range_zero_count = 0;                                      \
        }                                                                      \
                                                                               \
        for_every_lsb_zero_bit_rng(word, start_index, sizeof_bits(type), iter) \
        {                                                                      \
            if (iter.size < count) {                                           \
                /*
                 * If the range isn't long enough, but goes to the end of the
                 * word, then the next word can have the remaining zeroes needed
                 * in its lsb.
                 */                                                            \
                                                                               \
                if (range_get_end_assert(iter) == sizeof_bits(type)) {         \
                    current_range_start_index = bit_index_of_ptr + iter.front; \
                    current_range_zero_count = iter.size;                      \
                                                                               \
                    break;                                                     \
                }                                                              \
                                                                               \
                continue;                                                      \
            }                                                                  \
                                                                               \
            if (unset) {                                                       \
                bitmap_set_range(bitmap,                                       \
                                 RANGE_INIT(start_index, count),               \
                                 /*value=*/false);                             \
            }                                                                  \
                                                                               \
            return bit_index_of_ptr + iter.front;                              \
        }                                                                      \
                                                                               \
        start_index = 0;                                                       \
    }

    LOOP_OVER_RANGES_FOR_TYPE(uint64_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint32_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint16_t);
    LOOP_OVER_RANGES_FOR_TYPE(uint8_t);
#undef LOOP_OVER_RANGES_FOR_TYPE

    return true;
}

static uint64_t
find_single_unset(struct bitmap *const bitmap,
                  uint64_t start_index,
                  const bool set)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = (start_index % sizeof_bits(uint8_t));

    uint64_t bit_index_in_word = 0;
    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

#define ITERATE_FOR_TYPE(type)                                                 \
    for (;                                                                     \
         distance(ptr, end) >= sizeof(type); start_index = 0,                  \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type))           \
    {                                                                          \
        bit_index_in_word = find_lsb_zero_bit(*(type *)ptr, start_index);      \
        if (bit_index_in_word < sizeof_bits(type)) {                           \
            if (set) {                                                         \
                *(type *)ptr |= (type)1 << bit_index_in_word;                  \
            }                                                                  \
                                                                               \
            goto done;                                                         \
        }                                                                      \
    }

    ITERATE_FOR_TYPE(uint64_t);
    ITERATE_FOR_TYPE(uint32_t);
    ITERATE_FOR_TYPE(uint16_t);
    ITERATE_FOR_TYPE(uint8_t);
#undef ITERATE_FOR_TYPE

done:
    return bit_index_in_word + bit_index_of_ptr;
}

static uint64_t
find_single_set(struct bitmap *const bitmap,
                uint64_t start_index,
                const bool unset)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = (start_index % sizeof_bits(uint8_t));

    uint64_t bit_index_in_word = 0;
    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

#define ITERATE_FOR_TYPE(type)                                                 \
    for (;                                                                     \
         distance(ptr, end) >= sizeof(type); start_index = 0,                  \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type))           \
    {                                                                          \
        bit_index_in_word = find_lsb_one_bit(*(type *)ptr, start_index);       \
        if (bit_index_in_word < sizeof_bits(type)) {                           \
            if (unset) {                                                       \
                *(type *)ptr &= ~((type)1 << bit_index_in_word);               \
            }                                                                  \
                                                                               \
            goto done;                                                         \
        }                                                                      \
    }

    ITERATE_FOR_TYPE(uint64_t);
    ITERATE_FOR_TYPE(uint32_t);
    ITERATE_FOR_TYPE(uint16_t);
    ITERATE_FOR_TYPE(uint8_t);
#undef ITERATE_FOR_TYPE

done:
    return bit_index_in_word + bit_index_of_ptr;
}

__optimize(3) uint64_t
bitmap_find(struct bitmap *const bitmap,
            const uint64_t count,
            const uint64_t start_index,
            const bool expected_value,
            const bool invert)
{
    if (count != 1) {
        if (expected_value) {
            return find_multiple_set(bitmap, count, start_index, invert);
        }

        return find_multiple_unset(bitmap, count, start_index, invert);
    }

    if (expected_value) {
        return find_single_set(bitmap, start_index, invert);
    }

    return find_single_unset(bitmap, start_index, invert);
}

static uint64_t
find_unset_at_mult(struct bitmap *const bitmap,
                   const uint64_t count,
                   const uint64_t mult,
                   uint64_t start_index,
                   const bool set)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    if (!round_up(start_index, mult, &start_index)) {
        return false;
    }

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = start_index % sizeof_bits(uint8_t);

    /*
     * Loop over every word, and do the following:
     *   (a) Find every distinct sequence of zeros, and return if the sequence
     *       is large enough
     *   (b) If a sequence is not large enough, and is at the msb end of the
     *       word, check the leading zeros of the next word, and return if the
     *       combined sequence is large enough.
     */

    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

    uint64_t current_range_start_index = 0;
    uint64_t current_range_zero_count = 0;

#define ITERATE_FOR_TYPE(type)                                                 \
    for (;                                                                     \
         distance(ptr, end) >= sizeof(type);                                   \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type),           \
         start_index = 0)                                                      \
    {                                                                          \
        const type word = *(type *)ptr;                                        \
        if (current_range_zero_count != 0) {                                   \
            const uint8_t lsb_zero_count =                                     \
                min(sizeof_bits(type),                                         \
                    count_lsb_zero_bits(word, /*start_index=*/0));             \
                                                                               \
            if (current_range_zero_count + lsb_zero_count >= count) {          \
                if (set) {                                                     \
                    bitmap_set_range(bitmap,                                   \
                                     RANGE_INIT(start_index, count),           \
                                     /*value=*/true);                          \
                }                                                              \
                                                                               \
                return current_range_start_index;                              \
            }                                                                  \
                                                                               \
            if (lsb_zero_count == sizeof_bits(type)) {                         \
                current_range_zero_count += lsb_zero_count;                    \
                continue;                                                      \
            }                                                                  \
                                                                               \
            /*
             * If the lsb sequence of zeroes plus the msb zeros of the last word
             * wasn't long enough, the lsb sequence isn't long enough by itself.
             *
             * Avoid re-finding this same sequence of lsb zeros by pointing
             * start_index to the sequence's end.
             */                                                                \
                                                                               \
            start_index = lsb_zero_count;                                      \
            current_range_start_index = 0;                                     \
            current_range_zero_count = 0;                                      \
        }                                                                      \
                                                                               \
        for_every_lsb_zero_bit_rng(word,                                       \
                                   start_index,                                \
                                   sizeof_bits(type),                          \
                                   bad_iter)                                   \
        {                                                                      \
            struct range iter =                                                \
                RANGE_INIT(bit_index_of_ptr + bad_iter.front, bad_iter.size);  \
                                                                               \
            range_round_up_subrange(iter, mult, &iter);                        \
            if (iter.size < count) {                                           \
                /*
                 * If the range isn't long enough, but goes to the end of the
                 * word, then the next word can have the remaining zeroes needed
                 * in its lsb.
                 */                                                            \
                                                                               \
                if (range_get_end_assert(iter) == sizeof_bits(type)) {         \
                    current_range_start_index = bit_index_of_ptr + iter.front; \
                    current_range_zero_count = iter.size;                      \
                                                                               \
                    break;                                                     \
                }                                                              \
                                                                               \
                continue;                                                      \
            }                                                                  \
                                                                               \
            if (set) {                                                         \
                bitmap_set_range(bitmap,                                       \
                                 RANGE_INIT(iter.front, count),                \
                                 /*value=*/true);                              \
            }                                                                  \
                                                                               \
            return iter.front;                                                 \
        }                                                                      \
    }

    ITERATE_FOR_TYPE(uint64_t);
    ITERATE_FOR_TYPE(uint32_t);
    ITERATE_FOR_TYPE(uint16_t);
    ITERATE_FOR_TYPE(uint8_t);

#undef ITERATE_FOR_TYPE
    return FIND_BIT_INVALID;
}

static uint64_t
find_set_at_mult(struct bitmap *const bitmap,
                 const uint64_t count,
                 const uint64_t mult,
                 uint64_t start_index,
                 const bool unset)
{
    void *const begin = bitmap->gbuffer.begin;
    const void *const end = bitmap->gbuffer.end;

    void *ptr = begin + bits_to_bytes_noround(start_index);
    start_index = (start_index % sizeof_bits(uint8_t));

    if (!round_up(start_index, mult, &start_index)) {
        return false;
    }

    /*
     * Loop over every word, and do the following:
     *   (a) Find every distinct sequence of zeros, and return if the sequence
     *       is large enough
     *   (b) If a sequence is not large enough, and is at the msb end of the
     *       word, check the leading zeros of the next word, and return if the
     *       combined sequence is large enough.
     */

    uint64_t bit_index_of_ptr = bytes_to_bits(distance(begin, ptr));

    uint64_t current_range_start_index = 0;
    uint64_t current_range_zero_count = 0;

#define ITERATE_FOR_TYPE(type)                                                 \
    for (;                                                                     \
         distance(ptr, end) >= sizeof(type);                                   \
         ptr += sizeof(type), bit_index_of_ptr += sizeof_bits(type),           \
         start_index = 0)                                                      \
    {                                                                          \
        const type word = *(type *)ptr;                                        \
        if (current_range_zero_count != 0) {                                   \
            const uint8_t lsb_zero_count =                                     \
                min(sizeof_bits(type),                                         \
                    count_lsb_zero_bits(word, /*start_index=*/0));             \
                                                                               \
            if (current_range_zero_count + lsb_zero_count >= count) {          \
                if (unset) {                                                   \
                    bitmap_set_range(bitmap,                                   \
                                     RANGE_INIT(start_index, count),           \
                                     /*value=*/false);                         \
                }                                                              \
                                                                               \
                return current_range_start_index;                              \
            }                                                                  \
                                                                               \
            if (lsb_zero_count == sizeof_bits(type)) {                         \
                current_range_zero_count += lsb_zero_count;                    \
                continue;                                                      \
            }                                                                  \
                                                                               \
            /*
             * If the lsb sequence of zeroes plus the msb zeros of the last word
             * wasn't long enough, the lsb sequence isn't long enough by itself.
             *
             * Avoid re-finding this same sequence of lsb zeros by pointing
             * start_index to the sequence's end.
             */                                                                \
                                                                               \
            start_index = lsb_zero_count;                                      \
            current_range_start_index = 0;                                     \
            current_range_zero_count = 0;                                      \
        }                                                                      \
                                                                               \
        for_every_lsb_zero_bit_rng(word,                                       \
                                   start_index,                                \
                                   sizeof_bits(type),                          \
                                   bad_iter)                                   \
        {                                                                      \
            const struct range iter = RANGE_EMPTY();                           \
                /*loc_rng_create_sub_rng_mult_of(bad_iter, mult);*/            \
                                                                               \
            if (iter.size < count) {                                           \
                /*
                 * If the range isn't long enough, but goes to the end of the
                 * word, then the next word can have the remaining zeroes needed
                 * in its lsb.
                 */                                                            \
                                                                               \
                if (range_get_end_assert(iter) == sizeof_bits(type)) {         \
                    current_range_start_index = bit_index_of_ptr + iter.front; \
                    current_range_zero_count = iter.size;                      \
                                                                               \
                    break;                                                     \
                }                                                              \
                                                                               \
                continue;                                                      \
            }                                                                  \
                                                                               \
            if (unset) {                                                       \
                bitmap_set_range(bitmap,                                       \
                                 RANGE_INIT(start_index, count),               \
                                 /*value=*/false);                             \
            }                                                                  \
                                                                               \
            return bit_index_of_ptr + iter.front;                              \
        }                                                                      \
    }

    ITERATE_FOR_TYPE(uint64_t);
    ITERATE_FOR_TYPE(uint32_t);
    ITERATE_FOR_TYPE(uint16_t);
    ITERATE_FOR_TYPE(uint8_t);
#undef ITERATE_FOR_TYPE

    return FIND_BIT_INVALID;
}

__optimize(3) uint64_t
bitmap_find_at_mult(struct bitmap *const bitmap,
                    const uint64_t count,
                    const uint64_t mult,
                    const uint64_t start_index,
                    const bool expected_value,
                    const bool invert)
{
    if (expected_value) {
        return find_set_at_mult(bitmap, count, mult, start_index, invert);
    }

    return find_unset_at_mult(bitmap, count, mult, start_index, invert);
}

__optimize(3)
bool bitmap_at(const struct bitmap *const bitmap, uint64_t index) {
    void *const begin = bitmap->gbuffer.begin;
    void *ptr = begin + bits_to_bytes_noround(index);

    index = (index % sizeof_bits(uint8_t));
    return *(uint8_t *)ptr & (1 << index);
}

bool
bitmap_has(const struct bitmap *const bitmap,
           const struct range range,
           const bool value)
{
    assert(!range_empty(range));
    assert(index_range_in_bounds(range, bitmap_capacity(bitmap)));

    /*
     * Split process into 3 stages:
     *  1. Check values for bits in the first byte.
     *  2. For performance, simply check values for stream of bytes when n is
     *     large enough.
     *  3. Check values for bits in the last int.
     */

    void *const begin = bitmap->gbuffer.begin;
    void *ptr = begin + bits_to_bytes_noround(range.front);

    uint64_t count = range.size;

    const uint8_t first_byte_value_bits = value ? UINT8_MAX : 0;
    const uint64_t first_byte_bit_index = range.front % sizeof_bits(uint8_t);

    if (first_byte_bit_index != 0) {
        const uint8_t first_byte_shifted =
            *(uint8_t *)ptr >> first_byte_bit_index;
        const uint8_t first_byte_bits_count =
            min(range.size, sizeof_bits(uint8_t) - first_byte_bit_index);
        const uint8_t first_byte_bits =
            first_byte_shifted & mask_for_n_bits(first_byte_bits_count);
        const uint8_t first_byte_expected =
            first_byte_value_bits & mask_for_n_bits(first_byte_bits_count);

        if (first_byte_bits != first_byte_expected) {
            return false;
        }

        count -= first_byte_bits_count;
        if (count == 0) {
            return true;
        }

        ptr++;
    }

    /*
     * Stage 2: Check entire words if our n >= bit_sizeof(word-type)
     */

#define TYPE_FOR_BIT_AMT(bit_amount) VAR_CONCAT_3(uint, bit_amount, _t)
#define CHECK_MEMBUF_ALL_BITS_OF_VALUE(bit_amt)                                \
    do {                                                                       \
        const uint8_t word_count =                                             \
            bits_to_bytes_noround(count) / sizeof(TYPE_FOR_BIT_AMT(bit_amt));  \
                                                                               \
        if (word_count != 0) {                                                 \
            if (!VAR_CONCAT_3(membuf_, bit_amt, _is_all)(                      \
                    (TYPE_FOR_BIT_AMT(bit_amt) *)ptr,                          \
                    word_count,                                                \
                    value ? (TYPE_FOR_BIT_AMT(bit_amt))(~0) : 0))              \
            {                                                                  \
                return false;                                                  \
            }                                                                  \
                                                                               \
            count -=                                                           \
                bytes_to_bits(sizeof(TYPE_FOR_BIT_AMT(bit_amt)) * word_count); \
                                                                               \
            if (count == 0) {                                                  \
                return true;                                                   \
            }                                                                  \
        }                                                                      \
    } while (false)

    CHECK_MEMBUF_ALL_BITS_OF_VALUE(64);
    CHECK_MEMBUF_ALL_BITS_OF_VALUE(32);
    CHECK_MEMBUF_ALL_BITS_OF_VALUE(16);
    CHECK_MEMBUF_ALL_BITS_OF_VALUE(8);

#undef CHECK_MEMBUF_ALL_BITS_OF_VALUE
#undef TYPE_FOR_BIT_AMT

    // Stage 3: Check last byte.

    const uint8_t last_byte_bits = *(uint8_t *)ptr & mask_for_n_bits(count);
    const uint8_t last_byte_expected =
        first_byte_value_bits & mask_for_n_bits(count);

    return (last_byte_bits == last_byte_expected);
}

void bitmap_set(struct bitmap *const bitmap, uint64_t index, const bool value) {
    void *const begin = bitmap->gbuffer.begin;
    void *ptr = begin + bits_to_bytes_noround(index);

    index = (index % sizeof_bits(uint8_t));
    set_bits_for_mask((uint8_t *)ptr, 1ull << index, value);
}

void
bitmap_set_range(struct bitmap *const bitmap,
                 const struct range range,
                 const bool value)
{
    const uint64_t bit_index = range.front;

    /*
     * Split process into 3 stages:
     *  1. Set values for bits in the first int.
     *  2. Use memset() for performance to set entire bytes to 1 or 0 bits large
     *     enough.
     *  3. Set values for bits in the last int.
     *
     * Verify bits in the specified range are NOT of value (if requested),
     * before writing to the bitmap.
     */

    void *ptr = bitmap->gbuffer.begin + bits_to_bytes_noround(bit_index);
    uint64_t bits_left = range.size;

    /*
     * Only manually set first-byte if bit_index isn't zero, otherwise depend on
     * more performant memset().
     */

    const uint8_t first_byte_bit_index = bit_index % sizeof_bits(uint8_t);
    if (first_byte_bit_index != 0) {
        const uint64_t first_byte_bit_count =
            min(bits_left, sizeof_bits(uint8_t) - first_byte_bit_index);
        const uint8_t first_byte_mask =
            mask_for_n_bits(first_byte_bit_count) << first_byte_bit_index;

        set_bits_for_mask((uint8_t *)ptr, first_byte_mask, value);

        bits_left -= first_byte_bit_count;
        ptr++;
    }

    // Use memset() if we have streams of bytes.
    const uint64_t memset_byte_count = bits_to_bytes_noround(bits_left);
    if (memset_byte_count != 0) {
        if (value) {
            memset_all_ones(ptr, memset_byte_count);
        } else {
            bzero(ptr, memset_byte_count);
        }

        bits_left -= bytes_to_bits(memset_byte_count);
        ptr += memset_byte_count;
    }

    // Set values for last byte.
    if (bits_left != 0) {
        set_bits_for_mask((uint8_t *)ptr, mask_for_n_bits(bits_left), value);
    }
}

void bitmap_set_all(struct bitmap *const bitmap, const bool value) {
    if (value) {
        const uint64_t capacity = gbuffer_capacity(bitmap->gbuffer);
        memset_all_ones(bitmap->gbuffer.begin, capacity);
    } else {
        bzero(bitmap->gbuffer.begin, gbuffer_capacity(bitmap->gbuffer));
    }
}

void bitmap_destroy(struct bitmap *const bitmap) {
    gbuffer_destroy(&bitmap->gbuffer);
}

