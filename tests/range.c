/*
 * tests/range.c
 * © suhas pai
 */

#include <lib/adt/range.h>
#include <lib/macros.h>
#include <stdint.h>

#include "common.h"

static const struct range range_list[] = {
    RANGE_INIT(0, 0),
    RANGE_INIT(1, 0),
    RANGE_INIT(0, 1),
    RANGE_INIT(10, 1),
};

static bool verify_multiply(const struct range range) {
    struct range multiplied = RANGE_EMPTY();
    bool result = true;

    check_number_set_result(range_multiply(range, 2, &multiplied),
                            true,
                            result);

    check_number_set_result(multiplied.front, range.front * 2, result);
    check_number_set_result(multiplied.size, range.size * 2, result);

    return true;
}

__debug_optimize(3) static bool verify_divide(const struct range range) {
    static const uint64_t division_list[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    bool result = true;

    carr_foreach(division_list, divisor) {
        struct range divided = range_divide_out(range, *divisor);
        const uint64_t rounded_up = div_round_up(range.size, *divisor);

        check_number_set_result(divided.front, range.front / *divisor, result);
        check_number_set_result(divided.size, rounded_up, result);
    }

    return result;
}

static bool verify_align(const struct range range) {
    struct range aligned = RANGE_EMPTY();
    static const uint64_t alignment_list[] = {
        4, 8, 16, 32, 64, 128, 256, 512, 1024
    };

    bool result = true;
    carr_foreach(alignment_list, alignment) {
        uint64_t front_align = 0;
        uint64_t size_align = 0;

        if (range.size >= *alignment) {
            check_number_set_result(range_align_in(range, *alignment, &aligned),
                                    true,
                                    result);

            front_align = aligned.front % *alignment;
            size_align = aligned.size % *alignment;

            check_number_set_result(front_align, 0, result);
            check_number_set_result(size_align, 0, result);
        } else {
            result =
                check_number(range_align_in(range, *alignment, &aligned),
                             false);
        }

        check_number_set_result(range_align_out(range, *alignment, &aligned),
                                true,
                                result);

        check_number_set_result(front_align, 0, result);
        check_number_set_result(size_align, 0, result);
    }

    return result;
}

static bool verify_has_index(const struct range range) {
    bool result = true;
    if (range.size == 0) {
        check_number_set_result(range_has_index(range, 0), false, result);
        return result;
    }

    for_upto_limit(range.size, i) {
        check_number_set_result(range_has_index(range, i), true, result);
    }

    check_number_set_result(range_has_index(range, range.size), false, result);
    check_number_set_result(range_has_index(range, range.size + 1),
                            false,
                            result);

    return result;
}

static bool verify_has_loc(const struct range range) {
    bool result = true;
    if (range.size == 0) {
        check_number_set_result(range_has_loc(range, range.front), false,
                                result);
        check_number_set_result(range_has_loc(range, range.front + range.size),
                                false,
                                result);

        return result;
    }

    for_upto_limit(range.size, i) {
        check_number_set_result(range_has_loc(range, range.front + i), true, result);
    }

    return result;
}

static bool verify_has_end(const struct range range) {
    bool result = true;
    if (range.size == 0) {
        check_number_set_result(range_has_end(range, range.front),
                                false,
                                result);
        return result;
    }

    uint64_t end = 0;
    check_number(range_get_end(range, &end), true);

    check_number(range_has_end(range, range.front), false);
    if (end - 1 != range.front) {
        check_number_set_result(range_has_end(range, end - 1), true, result);
    }

    check_number_set_result(range_has_end(range, end), true, result);
    return result;
}

void test_range() {
    bool result = true;
    carr_foreach(range_list, range) {
        set_if_not(result, verify_multiply(*range));
        set_if_not(result, verify_divide(*range));
        set_if_not(result, verify_align(*range));

        set_if_not(result, verify_has_index(*range));
        set_if_not(result, verify_has_loc(*range));
        set_if_not(result, verify_has_end(*range));
    }

    if (result) {
        printf("range: All tests passed!\n");
    }
}
