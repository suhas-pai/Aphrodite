/*
 * tests/convert.c
 * © suhas pai
 */

#include <assert.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "lib/convert.h"

struct str_to_num_test {
    const char *string;
    struct str_to_num_options options;
    bool is_signed;
    enum str_to_num_result expected_result;
    uint64_t expected_number;
    uint64_t expected_end_index; // UINT64_MAX if NULL
};

static const struct str_to_num_test str_to_num_test_list[] = {
    {
        .string = "0",
        .options = { .default_base = NUMERIC_BASE_10 },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "        0",
        .options = { .default_base = NUMERIC_BASE_10 },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_INVALID_CHAR,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "        0",
        .options = {
            .default_base = NUMERIC_BASE_10,
            .skip_leading_whitespace = true,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "        0      ",
        .options = {
            .default_base = NUMERIC_BASE_10,
            .skip_leading_whitespace = true,
            .dont_parse_to_end = true,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 0,
        .expected_end_index = 9,
    },
    {
        .string = "        0a     ",
        .options = {
            .default_base = NUMERIC_BASE_10,
            .skip_leading_whitespace = true,
            .dont_allow_0a_prefix = true
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_UNALLOWED_PREFIX,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "10",
        .options = {
            .default_base = NUMERIC_BASE_2,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 2,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "11",
        .options = {
            .default_base = NUMERIC_BASE_2,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 3,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "11",
        .options = {
            .default_base = NUMERIC_BASE_10,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 11,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0b1",
        .options = {
            .default_base = NUMERIC_BASE_2,
            .dont_allow_0b_prefix = true
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_UNALLOWED_PREFIX,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0b1",
        .options = {
            .default_base = NUMERIC_BASE_2,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 1,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0B1",
        .options = {
            .default_base = NUMERIC_BASE_2,
            .dont_allow_0B_prefix = true
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_UNALLOWED_PREFIX,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0B1",
        .options = {
            .default_base = NUMERIC_BASE_2,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 1,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0b100",
        .options = {
            .default_base = NUMERIC_BASE_2,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 4,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0o3",
        .options = {
            .default_base = NUMERIC_BASE_8,
            .dont_allow_0o_prefix = true,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_UNALLOWED_PREFIX,
        .expected_number = 0,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0o3",
        .options = {
            .default_base = NUMERIC_BASE_8,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 3,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0o17",
        .options = {
            .default_base = NUMERIC_BASE_8,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 15,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "6",
        .options = {
            .default_base = NUMERIC_BASE_10,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 6,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0x6",
        .options = {
            .default_base = NUMERIC_BASE_16,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 6,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0X18",
        .options = {
            .default_base = NUMERIC_BASE_16,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 24,
        .expected_end_index = UINT64_MAX
    },
    {
        .string = "0x18",
        .options = {
            .default_base = NUMERIC_BASE_16,
        },
        .is_signed = false,
        .expected_result = E_STR_TO_NUM_OK,
        .expected_number = 24,
        .expected_end_index = UINT64_MAX
    },
};

struct num_to_str_test {
    uint64_t number;

    const char *const expected_unsigned_binary;
    const char *const expected_signed_binary;

    const char *const expected_unsigned_octal;
    const char *const expected_signed_octal;

    const char *const expected_unsigned_decimal;
    const char *const expected_signed_decimal;

    const char *const expected_unsigned_hexadecimal;
    const char *const expected_signed_hexadecimal;

    const char *const expected_unsigned_alphabetic;
    const char *const expected_signed_alphabetic;
};

static const struct num_to_str_test num_to_str_test_list[] = {
    {
        .number = 0,

        .expected_unsigned_binary = "0",
        .expected_signed_binary = "0",

        .expected_unsigned_octal = "0",
        .expected_signed_octal = "0",

        .expected_unsigned_decimal = "0",
        .expected_signed_decimal = "0",

        .expected_unsigned_hexadecimal = "0",
        .expected_signed_hexadecimal = "0",

        .expected_unsigned_alphabetic = "0",
        .expected_signed_alphabetic = "0",
    },
    {
        .number = 3,

        .expected_unsigned_binary = "11",
        .expected_signed_binary = "11",

        .expected_unsigned_octal = "3",
        .expected_signed_octal = "3",

        .expected_unsigned_decimal = "3",
        .expected_signed_decimal = "3",

        .expected_unsigned_hexadecimal = "3",
        .expected_signed_hexadecimal = "3",

        .expected_unsigned_alphabetic = "3",
        .expected_signed_alphabetic = "3",
    },
    {
        .number = 65326312,

        .expected_unsigned_binary = "11111001001100110011101000",
        .expected_signed_binary = "11111001001100110011101000",

        .expected_unsigned_octal = "371146350",
        .expected_signed_octal = "371146350",

        .expected_unsigned_decimal = "65326312",
        .expected_signed_decimal = "65326312",

        .expected_unsigned_hexadecimal = "3E4CCE8",
        .expected_signed_hexadecimal = "3E4CCE8",

        .expected_unsigned_alphabetic = "12W63S",
        .expected_signed_alphabetic = "12W63S",
    },
    {
        .number = (uint64_t)-1322545, // 18446744073708229071

        .expected_unsigned_binary =
            "1111111111111111111111111111111111111111111010111101000111001111",
        .expected_signed_binary = "-101000010111000110001",

        .expected_unsigned_octal = "1777777777777772750717",
        .expected_signed_octal = "-5027061",

        .expected_unsigned_decimal = "18446744073708229071",
        .expected_signed_decimal = "-1322545",

        .expected_unsigned_hexadecimal = "FFFFFFFFFFEBD1CF",
        .expected_signed_hexadecimal = "-142E31",

        .expected_unsigned_alphabetic = "3W5E1126404B3",
        .expected_signed_alphabetic = "-schd",
    },
    {
        .number = UINT64_MAX,

        .expected_unsigned_binary =
            "1111111111111111111111111111111111111111111111111111111111111111",
        .expected_signed_binary = "-1",
        .expected_unsigned_octal = "1777777777777777777777",
        .expected_signed_octal = "-1",

        .expected_unsigned_decimal = "18446744073709551615",
        .expected_signed_decimal = "-1",

        .expected_unsigned_hexadecimal = "FFFFFFFFFFFFFFFF",
        .expected_signed_hexadecimal = "-1",

        .expected_unsigned_alphabetic = "3W5E11264SGSF",
        .expected_signed_alphabetic = "-1"
    }
};

void run_str_to_num_test(const struct str_to_num_test *const test) {
    const char *end = NULL;
    if (test->is_signed) {
        int64_t number = 0;
        enum str_to_num_result result =
            cstr_to_signed(test->string, test->options, &end, &number);

        assert(result == test->expected_result);
        assert(number == (int64_t)test->expected_number);

        if (test->expected_end_index != UINT64_MAX) {
            assert(end == (test->string + test->expected_end_index));
        }

        struct string_view sv_end = SV_EMPTY();
        result =
            sv_to_signed(sv_create(test->string),
                         test->options,
                         &sv_end,
                         &number);

        assert(result == test->expected_result);
        assert(number == test->expected_number);

        if (test->expected_end_index != UINT64_MAX) {
            assert(sv_end.begin == (test->string + test->expected_end_index));
        }
    } else {
        uint64_t number = 0;
        enum str_to_num_result result =
            cstr_to_unsigned(test->string, test->options, &end, &number);

        assert(result == test->expected_result);
        assert(number == test->expected_number);

        if (test->expected_end_index != UINT64_MAX) {
            assert(end == (test->string + test->expected_end_index));
        }

        struct string_view sv_end = SV_EMPTY();
        result =
            sv_to_unsigned(sv_create(test->string),
                           test->options,
                           &sv_end,
                           &number);

        assert(result == test->expected_result);
        assert(number == test->expected_number);

        if (test->expected_end_index != UINT64_MAX) {
            assert(sv_end.begin == (test->string + test->expected_end_index));
        }
    }
}

void run_num_to_str_test(const struct num_to_str_test *const test) {
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view binary_unsigned_sv =
            unsigned_to_string_view(test->number,
                                    NUMERIC_BASE_2,
                                    buffer,
                                    NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(binary_unsigned_sv,
                               test->expected_unsigned_binary));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view octal_unsigned_sv =
            unsigned_to_string_view(test->number,
                                    NUMERIC_BASE_8,
                                    buffer,
                                    NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(octal_unsigned_sv,
                               test->expected_unsigned_octal));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view decimal_unsigned_sv =
            unsigned_to_string_view(test->number,
                                    NUMERIC_BASE_10,
                                    buffer,
                                    NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(decimal_unsigned_sv,
                               test->expected_unsigned_decimal));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view hexadecimal_unsigned_sv =
            unsigned_to_string_view(test->number,
                                    NUMERIC_BASE_16,
                                    buffer,
                                    (struct num_to_str_options){
                                        .capitalize = true
                                    });

        assert(sv_equals_c_str(hexadecimal_unsigned_sv,
                               test->expected_unsigned_hexadecimal));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view binary_signed_sv =
            signed_to_string_view((int64_t)test->number,
                                  NUMERIC_BASE_2,
                                  buffer,
                                  NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(binary_signed_sv,
                               test->expected_signed_binary));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view octal_signed_sv =
            signed_to_string_view((int64_t)test->number,
                                  NUMERIC_BASE_8,
                                  buffer,
                                  NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(octal_signed_sv,
                               test->expected_signed_octal));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view decimal_signed_sv =
            signed_to_string_view((int64_t)test->number,
                                  NUMERIC_BASE_10,
                                  buffer,
                                  NUM_TO_STR_OPTIONS_INIT());

        assert(sv_equals_c_str(decimal_signed_sv,
                               test->expected_signed_decimal));
    }
    {
        char buffer[MAX_CONVERT_CAP] = {0};
        const struct string_view hexadecimal_signed_sv =
            signed_to_string_view((int64_t)test->number,
                                  NUMERIC_BASE_16,
                                  buffer,
                                  (struct num_to_str_options){
                                      .capitalize = true
                                  });

        assert(sv_equals_c_str(hexadecimal_signed_sv,
                               test->expected_signed_hexadecimal));
    }
}

void test_convert() {
    for_each_in_carr (str_to_num_test_list, test) {
        run_str_to_num_test(test);
    }

    for_each_in_carr (num_to_str_test_list, test) {
        run_num_to_str_test(test);
    }
}