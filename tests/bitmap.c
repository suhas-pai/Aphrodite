/*
 * tests/bitmap.c
 * © suhas pai
 */

#include "lib/adt/bitmap.h"
#include "common.h"

void
set_and_check_index(struct bitmap *const bitmap, const uint64_t index) {
    bitmap_set(bitmap, index, true);
    assert(bitmap_at(bitmap, index));
    bitmap_set(bitmap, index, false);
    assert(!bitmap_at(bitmap, index));
}

void
set_and_check_range(struct bitmap *const bitmap,
                    const struct range range)
{
    bitmap_set_range(bitmap, range, true);
    assert(bitmap_has(bitmap, range, true));
    bitmap_set_range(bitmap, range, false);
    assert(bitmap_has(bitmap, range, false));

    bitmap_set_all(bitmap, /*value=*/false);
}

void test_cont(struct bitmap *const bitmap, const uint64_t count) {
    uint64_t bit_index = 0;
    assert(
        bitmap_find(bitmap,
                    count,
                    /*start_index=*/123,
                    /*expected_value=*/false,
                    /*invert=*/true));

    const struct range range = RANGE_INIT(bit_index, count);

    assert(bitmap_has(bitmap, range, true));
    bitmap_set_range(bitmap, range, false);
    assert(bitmap_has(bitmap, range, false));
}

void
test_at_mult(struct bitmap *const bitmap,
             const uint64_t mult,
             const uint64_t count)
{
#if 0
    uint64_t bit_index = 0;
    assert(
        bitmap_find_at_mult(bitmap,
                            count,
                            mult,
                            /*search_start_bit_index=*/123,
                            /*expected_value=*/false,
                            /*set=*/true));

    const struct range range = RANGE_INIT(bit_index, count);

    assert(bitmap_has(*bitmap, range, true));
    assert(bitmap_set(bitmap, range, false, true));
    assert(bitmap_has(*bitmap, range, false));
#endif
}

extern void *malloc(size_t size);
void check_bitmap(const uint64_t size) {
    void *const buffer = malloc(sizeof(uint8_t) * size);
    struct bitmap bitmap = bitmap_open(buffer, size);

/*
    assert(bitmap_has_index(bitmap, 0));
    assert(bitmap_has_index(bitmap, 1));
    assert(bitmap_has_index(bitmap, 7));
    assert(bitmap_has_index(bitmap, 0));
    assert(bitmap_has_index(bitmap, byte_to_bit_amount(size) - 1));
    assert(!bitmap_has_index(bitmap, byte_to_bit_amount(size)));
    assert(!bitmap_has_index(bitmap, byte_to_bit_amount(size)) + 1); */

    set_and_check_index(&bitmap, 0);
    set_and_check_index(&bitmap, 1);
    set_and_check_index(&bitmap, 7);
    set_and_check_range(&bitmap, range_create_end(0, 1));
    set_and_check_range(&bitmap, range_create_end(1, 2));
    set_and_check_range(&bitmap, range_create_end(1, 3));
    set_and_check_range(&bitmap, range_create_end(2, 3));
    set_and_check_range(&bitmap, range_create_end(0, 7));
    set_and_check_range(&bitmap, range_create_end(4, 5));
    set_and_check_range(&bitmap, range_create_end(4, 6));
    set_and_check_range(&bitmap, range_create_end(4, 7));

    if (size > 1) {
        set_and_check_index(&bitmap, 8);
        set_and_check_index(&bitmap, 9);
        set_and_check_index(&bitmap, 15);

        set_and_check_range(&bitmap, range_create_end(0, 8));
        set_and_check_range(&bitmap, range_create_end(0, 9));
        set_and_check_range(&bitmap, range_create_end(5, 12));
        set_and_check_range(&bitmap, range_create_end(7, 13));
        set_and_check_range(&bitmap, range_create_end(0, 15));

        if (size > 2) {
            set_and_check_index(&bitmap, 16);
            set_and_check_index(&bitmap, 23);
            set_and_check_range(&bitmap, range_create_end(0, 16));
            set_and_check_range(&bitmap, range_create_end(0, 23));

            if (size > 3) {
                set_and_check_range(&bitmap, range_create_end(0, 39));
            }

            if (size > 1000) {
                set_and_check_range(&bitmap, range_create_end(0, 1000));
                set_and_check_range(&bitmap, range_create_end(5433, 6655));
            }
        }
    }

    bitmap_set_all(&bitmap, /*value=*/false);
    {
        uint64_t unset_index =
            bitmap_find(&bitmap,
                        /*count=*/1,
                        /*start_index=*/0,
                        /*expected_value=*/false,
                        /*invert=*/true);

        if (unset_index != FIND_BIT_INVALID) {
            assert(bitmap_at(&bitmap, unset_index));
            bitmap_set(&bitmap, unset_index, false);
        }
    }

#define CHECK_UNSET(index)                                                     \
    assert(bitmap_find(&bitmap, 1, index, false, false) == (index))

    CHECK_UNSET(0);
    CHECK_UNSET(1);
    CHECK_UNSET(4);
    CHECK_UNSET(7);

    if (size > 1) {
        CHECK_UNSET(8);
        CHECK_UNSET(9);
        CHECK_UNSET(15);
    }

    bitmap_set_all(&bitmap, /*value=*/true);
    if (size > 1) {
        {
            const uint64_t unset_index = 3;
            const uint64_t unset_count = 8;

            for (uint8_t i = unset_index; i != unset_count; i++) {
                bitmap_set(&bitmap, i, true);
            }

            const uint64_t actual_unset_index =
                bitmap_find(&bitmap,
                            unset_count,
                            unset_index,
                            /*expected_value=*/true,
                            /*invert=*/false);

            assert(actual_unset_index != FIND_BIT_INVALID);
            const struct range unset_range =
                RANGE_INIT(unset_index, unset_count);

            assert(bitmap_has(&bitmap, unset_range, /*value=*/true));
        }
    }
    if (size > 2) {
        {
            const struct range unset_range = RANGE_INIT(12, 20);

            bitmap_set_all(&bitmap, /*value=*/true);
            bitmap_set_range(&bitmap, unset_range, /*value=*/false);

            const uint64_t actual_unset_index =
                bitmap_find_at_mult(&bitmap,
                                    /*n=*/unset_range.size,
                                    /*mult=*/unset_range.front,
                                    /*start_index=*/0,
                                    /*expected_value=*/false,
                                    /*invert=*/true);

            assert(actual_unset_index != FIND_BIT_INVALID);
            assert(bitmap_has(&bitmap, unset_range, /*value=*/true));
        }
    }
    if (size > 7) {
        {
            const struct range unset_range = RANGE_INIT(56, 8);

            bitmap_set_all(&bitmap, /*value=*/true);
            bitmap_set_range(&bitmap, unset_range, /*value=*/false);

            const uint64_t actual_unset_index =
                bitmap_find_at_mult(&bitmap,
                                    /*count=*/8,
                                    /*mult=*/8,
                                    /*start_index=*/0,
                                    /*expected_value=*/false,
                                    /*invert=*/true);

            assert(actual_unset_index != FIND_BIT_INVALID);
            assert(bitmap_has(&bitmap, unset_range, /*value=*/true));
        }
    }
    #if 0
    {
        bitmap_set_all_bits(&bitmap);
        const struct loc_range set_bit_rng =
            bitmap_get_loc_rng_of_set_bits(&bitmap, 0, /*unset=*/false);
        const struct loc_range lr =
            loc_rng_create_from_size_rng(bitmap_get_bit_size_rng(bitmap));

        assert(loc_rng_is_equal(set_bit_rng, lr));
    }
    {
        bitmap_unset_all_bits(&bitmap);
        const struct loc_range unset_bit_rng =
            bitmap_get_loc_rng_of_unset_bits(&bitmap, 0, /*set=*/true);
        const struct loc_range lr =
            loc_rng_create_from_size_rng(bitmap_get_bit_size_rng(bitmap));

        assert(loc_rng_is_equal(unset_bit_rng, lr));
        assert(bitmap_is_all_set(bitmap));
    }
    {
        bitmap_set_all_bits(&bitmap);
        const struct loc_range set_bit_rng =
            bitmap_get_loc_rng_of_set_bits(&bitmap, 0, /*unset=*/true);
        const struct loc_range lr =
            loc_rng_create_from_size_rng(bitmap_get_bit_size_rng(bitmap));

        assert(loc_rng_is_equal(set_bit_rng, lr));
        assert(bitmap_is_all_unset(bitmap));
    }
    if (size > 4) {
        {
            const struct loc_range unset_rng = loc_rng_create_with_end(12, 20);

            bitmap_set_all_bits(&bitmap);
            bitmap_set_value_for_loc_rng(&bitmap, unset_rng, false, true);

            const struct loc_range unset_bit_rng =
                bitmap_get_loc_rng_of_unset_bits(&bitmap, 0, /*set=*/true);

            assert(bitmap_has_value_for_loc_rng(bitmap, unset_rng, true));
            assert(loc_rng_is_equal(unset_rng, unset_bit_rng));
        }
        {
            const struct loc_range set_rng = loc_rng_create_with_end(12, 20);

            bitmap_unset_all_bits(&bitmap);
            bitmap_set_value_for_loc_rng(&bitmap, set_rng, true, true);

            const struct loc_range set_bit_rng =
                bitmap_get_loc_rng_of_set_bits(&bitmap, 0, /*unset=*/true);

            assert(bitmap_has_value_for_loc_rng(bitmap, set_rng, false));
            assert(loc_rng_is_equal(set_rng, set_bit_rng));
        }
    }
    if (size > 8) {
        {
            bitmap_set_all_bits(&bitmap);

            const uint64_t unset_index = bit_sizeof(uint64_t) - 5;
            const uint64_t unset_count = 8;

            for_each_begin_to_count (u8int, i, unset_index, unset_count) {
                bitmap_set_value_at_index(&bitmap, i, false, true);
            }

            const struct loc_range unset_rng =
                loc_rng_create_with_size(unset_index, unset_count);
            const struct loc_range unset_bit_rng =
                bitmap_get_loc_rng_of_unset_bits(&bitmap, 0, /*set=*/true);

            assert(bitmap_has_value_for_loc_rng(bitmap, unset_rng, true));
            assert(loc_rng_is_equal(unset_rng, unset_bit_rng));
        }
        {
            bitmap_unset_all_bits(&bitmap);

            const uint64_t set_index = bit_sizeof(uint64_t) - 5;
            const uint64_t set_count = 8;

            for_each_begin_to_count (u8int, i, set_index, set_count) {
                bitmap_set_value_at_index(&bitmap, i, true, true);
            }

            const struct loc_range set_rng =
                loc_rng_create_with_size(set_index, set_count);
            const struct loc_range set_bit_rng =
                bitmap_get_loc_rng_of_set_bits(&bitmap, 0, /*unset=*/true);

            assert(bitmap_has_value_for_loc_rng(bitmap, set_rng, false));
            assert(loc_rng_is_equal(set_rng, set_bit_rng));
        }
    }
    if (size > 1000) {
        bitmap_unset_all_bits(&bitmap);
        test_at_mult(&bitmap, 100, 500);
        bitmap_unset_all_bits(&bitmap);
        test_cont(&bitmap, 513);
    }
    #endif

    free(buffer);
}

void test_bitmap() {
    check_bitmap(1);
    check_bitmap(2);
    check_bitmap(1024);
    check_bitmap(163840);
}