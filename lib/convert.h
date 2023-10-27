/*
 * lib/convert.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/string_view.h"

enum numeric_base {
    NUMERIC_BASE_2 = 2,
    NUMERIC_BASE_8 = 8,
    NUMERIC_BASE_10 = 10,
    NUMERIC_BASE_16 = 16,
    NUMERIC_BASE_36 = 36,
};

struct str_to_num_options {
    enum numeric_base default_base : 8;

    bool dont_allow_base_2 : 1;
    bool dont_allow_base_8 : 1;
    bool dont_allow_base_10 : 1;
    bool dont_allow_base_16 : 1;
    bool dont_allow_base_36 : 1;

    bool dont_allow_0a_prefix : 1;
    bool dont_allow_0A_prefix : 1;

    bool dont_allow_0b_prefix : 1;
    bool dont_allow_0B_prefix : 1;

    bool dont_allow_0o_prefix : 1;
    bool dont_allow_0O_prefix : 1;
    bool dont_allow_0_prefix  : 1;

    bool dont_allow_0x_prefix : 1;
    bool dont_allow_0X_prefix : 1;

    // Leading zeros after the prefix, should not be set with allow_0_prefix
    bool allow_leading_zeros : 1;
    bool dont_allow_pos_sign : 1;
    bool skip_leading_whitespace : 1;

    // Parse to end-of-string, or return E_STR_TO_NUM_INVALID_CHAR
    bool dont_parse_to_end : 1;
};

enum str_to_num_result {
    E_STR_TO_NUM_OK,

    E_STR_TO_NUM_NO_DIGITS,
    E_STR_TO_NUM_INVALID_CHAR,
    E_STR_TO_NUM_NEG_UNSIGNED,

    E_STR_TO_NUM_UNALLOWED_PREFIX,
    E_STR_TO_NUM_UNALLOWED_BASE,
    E_STR_TO_NUM_UNALLOWED_POS_SIGN,

    E_STR_TO_NUM_UNDERFLOW,
    E_STR_TO_NUM_OVERFLOW,
};

enum str_to_num_result
cstr_to_unsigned(const char *c_str,
                 const struct str_to_num_options options,
                 const char **end_out,
                 uint64_t *result_out);

enum str_to_num_result
cstr_to_signed(const char *c_str,
               const struct str_to_num_options options,
               const char **end_out,
               int64_t *result_out);

enum str_to_num_result
sv_to_unsigned(struct string_view sv,
               struct str_to_num_options options,
               struct string_view *end_out,
               uint64_t *result_out);

enum str_to_num_result
sv_to_signed(struct string_view sv,
             struct str_to_num_options options,
             struct string_view *end_out,
             int64_t *result_out);

#define MAX_BINARY_CONVERT_CAP 68
#define MAX_OCTAL_CONVERT_CAP 26
#define MAX_DECIMAL_CONVERT_CAP 22
#define MAX_HEXADECIMAL_CONVERT_CAP 20
#define MAX_ALPHABETIC_CONVERT_CAP 20

#define MAX_CONVERT_CAP MAX_BINARY_CONVERT_CAP

struct num_to_str_options {
    bool include_prefix : 1;
    bool include_pos_sign : 1;

    bool capitalize : 1;
    bool capitalize_prefix : 1;

    bool use_0_octal_prefix : 1;
};

#define NUM_TO_STR_OPTIONS_INIT() \
    ((struct num_to_str_options){ \
        .include_prefix = false, \
        .include_pos_sign = false, \
        .capitalize = false, \
        .capitalize_prefix = false, \
        .use_0_octal_prefix = false \
    })

struct string_view
unsigned_to_string_view(uint64_t number,
                        enum numeric_base base,
                        char buffer[static MAX_CONVERT_CAP],
                        struct num_to_str_options options);

struct string_view
signed_to_string_view(int64_t number,
                      enum numeric_base base,
                      char buffer[static MAX_CONVERT_CAP],
                      struct num_to_str_options options);