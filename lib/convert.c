/*
 * lib/convert.c
 * © suhas pai
 */

#include <limits.h>
#include "lib/util.h"

#include "convert.h"
#include "ctype.h"

__optimize(3) enum str_to_num_result
convert_cstr_to_64int(const char *string,
                      const struct str_to_num_options options,
                      const bool is_signed,
                      const char **const end_out,
                      uint64_t *const result_out)
{
    char ch = *string;
    if (ch == '\0') {
        return E_STR_TO_NUM_NO_DIGITS;
    }

    if (options.skip_leading_whitespace) {
        do {
            if (isspace(ch) == 0) {
                break;
            }

            ch = *(++string);
            if (ch == '\0') {
                return E_STR_TO_NUM_NO_DIGITS;
            }
        } while (true);
    }

    // Check for zeros/leading-zeros/prefixes
    bool is_neg = false;
    if (ch == '-') {
        if (!is_signed) {
            return E_STR_TO_NUM_NEG_UNSIGNED;
        }

        ch = *(++string);
        if (ch == '\0') {
            return E_STR_TO_NUM_NO_DIGITS;
        }

        is_neg = true;
    } else if (ch == '+') {
        if (options.dont_allow_pos_sign) {
            return E_STR_TO_NUM_UNALLOWED_POS_SIGN;
        }

        ch = *(++string);
        if (ch == '\0') {
            return E_STR_TO_NUM_NO_DIGITS;
        }
    }

    // If we have just a zero, and no further digits, then the `!found_digits`
    // check will fail below, so we must account for that here.

    bool found_zero = false;
    uint8_t base = options.default_base;

    if (ch == '0') {
        ch = *(++string);
        switch (ch) {
            case '\0':
                *end_out = NULL;
                *result_out = 0;

                return E_STR_TO_NUM_OK;
            case 'a':
                if (options.dont_allow_0a_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_a;
            case 'A':
                if (options.dont_allow_0A_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_a:
                if (options.dont_allow_base_36) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 36;
                ch = *(++string);

                break;
            case 'b':
                if (options.dont_allow_0b_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_b;
            case 'B':
                if (options.dont_allow_0B_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_b:
                if (options.dont_allow_base_2) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 2;
                ch = *(++string);

                break;
            case 'o':
                if (options.dont_allow_0o_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_o;
            case 'O':
                if (options.dont_allow_0O_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_o:
                if (options.dont_allow_base_8) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 8;
                ch = *(++string);

                break;
            case 'x':
                if (options.dont_allow_0x_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_x;
            case 'X':
                if (options.dont_allow_0X_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_x:
                if (options.dont_allow_base_16) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 16;
                ch = *(++string);

                break;
            default:
                base = 8;
                found_zero = true;

                break;
        }
    }

    *result_out = 0;

    bool found_digit = found_zero;
    for (;; ch = *(++string)) {
        if (ch == '\0') {
            *end_out = NULL;
            break;
        }

        uint8_t digit = 255;
        if (isdigit(ch) != 0) {
            digit = ch - '0';
        } else if (isupper(ch) != 0) {
            digit = 10 + (ch - 'A');
        } else if (islower(ch) != 0) {
            digit = 10 + (ch - 'a');
        }

        if (digit >= base) {
            if (!options.dont_parse_to_end) {
                return E_STR_TO_NUM_INVALID_CHAR;
            }

            *end_out = string;
            break;
        }

        found_digit = true;
        if (!check_mul(*result_out, base, result_out) ||
            !check_add(*result_out, digit, result_out))
        {
            return E_STR_TO_NUM_OVERFLOW;
        }
    }

    if (!found_digit) {
        return E_STR_TO_NUM_NO_DIGITS;
    }

    if (is_signed) {
        if (*result_out > INT64_MAX) {
            return E_STR_TO_NUM_OVERFLOW;
        }
    }

    if (is_neg) {
        if (check_sub(0, *result_out, (int64_t *)result_out)) {
            return E_STR_TO_NUM_UNDERFLOW;
        }
    }

    return E_STR_TO_NUM_OK;
}

__optimize(3) enum str_to_num_result
convert_sv_to_64int(struct string_view sv,
                    const struct str_to_num_options options,
                    const bool is_signed,
                    struct string_view *const end_out,
                    uint64_t *const result_out)
{
    if (sv.length == 0) {
        return E_STR_TO_NUM_NO_DIGITS;
    }

    if (options.skip_leading_whitespace) {
        do {
            if (isspace(*sv.begin) == 0) {
                break;
            }

            sv = sv_drop_front(sv);
            if (sv.length == 0) {
                return E_STR_TO_NUM_NO_DIGITS;
            }
        } while (true);
    }

    // Check for zeros/leading-zeros/prefixes
    bool is_negative = false;
    if (*sv.begin == '-') {
        if (!is_signed) {
            return E_STR_TO_NUM_NEG_UNSIGNED;
        }

        sv = sv_drop_front(sv);
        if (sv.length == 0) {
            return E_STR_TO_NUM_NO_DIGITS;
        }

        is_negative = true;
    } else if (*sv.begin == '+') {
        if (options.dont_allow_pos_sign) {
            return E_STR_TO_NUM_UNALLOWED_POS_SIGN;
        }

        sv = sv_drop_front(sv);
        if (sv.length == 0) {
            return E_STR_TO_NUM_NO_DIGITS;
        }
    }

    // If we have just a zero, and no further digits, then the `!found_digits`
    // check will fail below, so we must account for that here.

    bool found_zero = false;
    uint8_t base = options.default_base;

    if (*sv.begin == '0') {
        sv = sv_drop_front(sv);
        if (sv.length == 0) {
            *end_out = sv;
            *result_out = 0;

            return E_STR_TO_NUM_OK;
        }

        switch (*sv.begin) {
            case 'a':
                if (options.dont_allow_0a_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_a;
            case 'A':
                if (options.dont_allow_0A_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_a:
                sv = sv_drop_front(sv);
                if (options.dont_allow_base_36) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 36;
                break;
            case 'b':
                if (options.dont_allow_0b_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_b;
            case 'B':
                if (options.dont_allow_0B_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_b:
                sv = sv_drop_front(sv);
                if (options.dont_allow_base_2) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 2;
                break;
            case 'o':
                if (options.dont_allow_0o_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_o;
            case 'O':
                if (options.dont_allow_0O_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_o:
                sv = sv_drop_front(sv);
                if (options.dont_allow_base_8) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 8;
                break;
            case 'x':
                if (options.dont_allow_0x_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

                goto common_x;
            case 'X':
                if (options.dont_allow_0X_prefix) {
                    return E_STR_TO_NUM_UNALLOWED_PREFIX;
                }

            common_x:
                sv = sv_drop_front(sv);
                if (options.dont_allow_base_16) {
                    return E_STR_TO_NUM_UNALLOWED_BASE;
                }

                base = 16;
                break;
            default:
                base = 8;
                found_zero = true;

                break;
        }
    }

    *result_out = 0;

    bool found_digit = found_zero;
    for (;; sv = sv_drop_front(sv)) {
        if (sv.length == 0) {
            *end_out = SV_EMPTY();
            break;
        }

        uint8_t digit = UINT8_MAX;
        char ch = *sv.begin;

        if (isdigit(ch)) {
            digit = ch - '0';
        } else if (isupper(ch)) {
            digit = 10 + (ch - 'A');
        } else if (islower(ch)) {
            digit = 10 + (ch - 'a');
        }

        if (digit >= base) {
            if (!options.dont_parse_to_end) {
                return E_STR_TO_NUM_INVALID_CHAR;
            }

            *end_out = sv;
            break;
        }

        found_digit = true;
        if (!check_mul(*result_out, base, result_out) ||
            !check_add(*result_out, digit, result_out))
        {
            return E_STR_TO_NUM_OVERFLOW;
        }
    }

    if (!found_digit) {
        return E_STR_TO_NUM_NO_DIGITS;
    }

    if (is_signed) {
        if (*result_out > INT64_MAX) {
            return E_STR_TO_NUM_OVERFLOW;
        }
    }

    if (is_negative) {
        if (check_sub(0, *result_out, (int64_t *)result_out)) {
            return E_STR_TO_NUM_UNDERFLOW;
        }
    }

    return E_STR_TO_NUM_OK;
}

enum str_to_num_result
cstr_to_unsigned(const char *const c_str,
                 const struct str_to_num_options options,
                 const char **const end_out,
                 uint64_t *const result_out)
{
    const enum str_to_num_result result =
        convert_cstr_to_64int(c_str,
                              options,
                              /*is_signed=*/false,
                              end_out,
                              result_out);
    return result;
}

enum str_to_num_result
cstr_to_signed(const char *const c_str,
               const struct str_to_num_options options,
               const char **const end_out,
               int64_t *const result_out)
{
    const enum str_to_num_result result =
        convert_cstr_to_64int(c_str,
                              options,
                              /*is_signed=*/true,
                              end_out,
                              (uint64_t *)result_out);
    return result;
}

enum str_to_num_result
sv_to_unsigned(const struct string_view sv,
               const struct str_to_num_options options,
               struct string_view *const end_out,
               uint64_t *const result_out)
{
    const enum str_to_num_result result =
        convert_sv_to_64int(sv,
                            options,
                            /*is_signed=*/false,
                            end_out,
                            result_out);
    return result;
}

enum str_to_num_result
sv_to_signed(const struct string_view sv,
             const struct str_to_num_options options,
             struct string_view *const end_out,
             int64_t *const result_out)
{
    const enum str_to_num_result result =
        convert_sv_to_64int(sv,
                            options,
                            /*is_signed=*/true,
                            end_out,
                            (uint64_t *)result_out);
    return result;
}

#define LOOP_OVER_POSITIVE_INT(base, mod_block)                                \
    do {                                                                       \
        const char *const h_var(chars) =                                       \
            (options.capitalize) ?                                             \
                get_alphanumeric_upper_string() :                              \
                get_alphanumeric_lower_string();                               \
                                                                               \
        buffer[i + 1] = '\0';                                                  \
        do {                                                                   \
            buffer[i] = h_var(chars)[number % (base)];                         \
            if (number < (base)) {                                             \
                break;                                                         \
            }                                                                  \
                                                                               \
            assert(i != 0);                                                    \
            i--;                                                               \
                                                                               \
            number /= (base);                                                  \
        } while (true);                                                        \
                                                                               \
        mod_block;                                                             \
        if (options.include_pos_sign) {                                        \
            i -= 1;                                                            \
            buffer[i] = '+';                                                   \
        }                                                                      \
    } while (false)

__optimize(3) struct string_view
unsigned_to_string_view(uint64_t number,
                        const enum numeric_base base,
                        char buffer[static const MAX_CONVERT_CAP],
                        const struct num_to_str_options options)
{
    uint8_t final_index = 0;
    switch (base) {
        case NUMERIC_BASE_2:
            final_index = MAX_BINARY_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_8:
            final_index = MAX_OCTAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_10:
            final_index = MAX_DECIMAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_16:
            final_index = MAX_HEXADECIMAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_36:
            final_index = MAX_CONVERT_CAP;
            break;
    }

    // Subtract one because our macro below will add a null-terminator,
    // so we want to be pointing one before the last index.

    uint8_t i = final_index - 1;
    LOOP_OVER_POSITIVE_INT(base, {
        if (options.include_prefix) {
            buffer[i - 2] = '0';
            switch (base) {
                case NUMERIC_BASE_2:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'B' : 'b';
                    break;
                case NUMERIC_BASE_8:
                    if (!options.use_0_octal_prefix) {
                        buffer[i - 1] = (options.capitalize_prefix) ? 'O' : 'o';
                    } else {
                        i += 1;
                    }

                    break;
                case NUMERIC_BASE_10:
                    break;
                case NUMERIC_BASE_16:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'X' : 'x';
                    break;
                case NUMERIC_BASE_36:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'A' : 'a';
                    break;
            }

            i -= 2;
        }
    });

    // Buffer + final_index points to the null-terminator, which is the end
    return sv_create_end(&buffer[i], buffer + final_index);
}

#define LOOP_OVER_SIGNED_INT(base, mod_block)                                  \
    do {                                                                       \
        if (number < 0) {                                                      \
            const char *const h_var(chars) =                                   \
                (options.capitalize) ?                                         \
                    get_alphanumeric_upper_string() :                          \
                    get_alphanumeric_lower_string();                           \
                                                                               \
            buffer[i + 1] = '\0';                                              \
            const int64_t h_var(neg_base) = (0 - (int64_t)(base));             \
            do {                                                               \
                buffer[i] = h_var(chars)[0 - (number % (base))];               \
                assert(i != 0);                                                \
                i--;                                                           \
                                                                               \
                if (number > h_var(neg_base)) {                                \
                    break;                                                     \
                }                                                              \
                                                                               \
                number /= (base);                                              \
            } while (number != 0);                                             \
                                                                               \
            mod_block;                                                         \
            buffer[i] = '-';                                                   \
        } else {                                                               \
            LOOP_OVER_POSITIVE_INT((base), mod_block);                         \
        }                                                                      \
    } while (false)

__optimize(3) struct string_view
signed_to_string_view(int64_t number,
                      const enum numeric_base base,
                      char buffer[static const MAX_CONVERT_CAP],
                      const struct num_to_str_options options)
{
    uint8_t final_index = 0;
    switch (base) {
        case NUMERIC_BASE_2:
            final_index = MAX_BINARY_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_8:
            final_index = MAX_OCTAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_10:
            final_index = MAX_DECIMAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_16:
            final_index = MAX_HEXADECIMAL_CONVERT_CAP - 1;
            break;
        case NUMERIC_BASE_36:
            final_index = MAX_ALPHABETIC_CONVERT_CAP - 1;
            break;
    }

    // Subtract one because our macro below will add a null-terminator,
    // so we want to be pointing one before the last index.

    uint8_t i = final_index - 1;
    LOOP_OVER_SIGNED_INT(base, {
        if (options.include_prefix) {
            buffer[i - 2] = '0';
            switch (base) {
                case NUMERIC_BASE_2:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'B' : 'b';
                    break;
                case NUMERIC_BASE_8:
                    if (!options.use_0_octal_prefix) {
                        buffer[i - 1] = (options.capitalize_prefix) ? 'O' : 'o';
                    } else {
                        i += 1;
                    }

                    break;
                case NUMERIC_BASE_10:
                    break;
                case NUMERIC_BASE_16:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'X' : 'x';
                    break;
                case NUMERIC_BASE_36:
                    buffer[i - 1] = (options.capitalize_prefix) ? 'A' : 'a';
                    break;
            }

            i -= 2;
        }
    });

    // Buffer + final_index points to the null-terminator, which is the end
    return sv_create_end(buffer + i, buffer + final_index);
}

unsigned long int
strtoul(const char *const str, char **const endptr, const int base) {
    uint64_t result = 0;
    struct str_to_num_options options = {
        .default_base = (enum numeric_base)base,
        .dont_allow_0A_prefix = true,
        .dont_allow_0a_prefix = true,
        .dont_allow_0B_prefix = true,
        .dont_allow_0b_prefix = true,
        .dont_allow_0o_prefix = true,
        .dont_allow_0O_prefix = true,
    };

    switch (base) {
        case 0:
            options.default_base = 10;
            break;
        case 2:
            options.dont_allow_base_8 = true;
            options.dont_allow_base_10 = true;
            options.dont_allow_base_16 = true;

            options.dont_allow_0b_prefix = false;
            options.dont_allow_0B_prefix = false;

            break;
        case 8:
            options.dont_allow_base_2 = true;
            options.dont_allow_base_10 = true;
            options.dont_allow_base_16 = true;

            options.dont_allow_0o_prefix = false;
            options.dont_allow_0_prefix = false;

            break;
        case 10:
            options.dont_allow_base_2 = true;
            options.dont_allow_base_8 = true;
            options.dont_allow_base_16 = true;

            break;
        case 16:
            options.dont_allow_base_2 = true;
            options.dont_allow_base_8 = true;
            options.dont_allow_base_10 = true;

            options.dont_allow_0x_prefix = false;
            options.dont_allow_0X_prefix = false;

            break;
        default:
            options.dont_allow_base_2 = true;
            options.dont_allow_base_8 = true;
            options.dont_allow_base_10 = true;
            options.dont_allow_base_16 = true;

            break;
    }

    const enum str_to_num_result error =
        cstr_to_unsigned(str,
                         options,
                         (const char **)(uint64_t)endptr,
                         &result);

    if (error != E_STR_TO_NUM_OK) {
        return ULONG_MAX;
    }

    return (unsigned long int)result;
}