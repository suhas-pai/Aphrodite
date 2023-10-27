/*
 * lib/parse_printf.c
 * © suhas pai
 */

#include "convert.h"
#include "parse_printf.h"

struct va_list_struct {
    va_list list;
};

__optimize(3) static bool
parse_flags(struct printf_spec_info *const curr_spec,
            const char *iter,
            const char **const iter_out)
{
    do {
        switch (*iter) {
            case ' ':
                curr_spec->add_one_space_for_sign = true;
                break;
            case '-':
                curr_spec->left_justify = true;
                break;
            case '+':
                curr_spec->add_pos_sign = true;
                break;
            case '#':
                curr_spec->add_base_prefix = true;
                break;
            case '0':
                curr_spec->leftpad_zeros = true;
                break;
            default:
                goto done;
        }

        iter++;
        if (*iter == '\0') {
            return false;
        }
    } while (true);

done:
    *iter_out = iter;
    return true;
}

__optimize(3) static inline int
read_int_from_fmt_string(const char *const c_str, const char **const iter_out) {
    int result = 0;
    c_string_foreach(c_str, iter) {
        const uint8_t digit = *iter - '0';
        if (digit >= 10) {
            *iter_out = iter;
            break;
        }

        if (!check_mul(result, 10, &result) ||
            !check_add(result, digit, &result))
        {
            *iter_out = iter;
            return -1;
        }
    }

    return result;
}

__optimize(3) static bool
parse_width(struct printf_spec_info *const curr_spec,
            struct va_list_struct *const list_struct,
            const char *iter,
            const char **const iter_out)
{
    if (*iter != '*') {
        const int width = read_int_from_fmt_string(iter, &iter);
        if (width == -1) {
            return false;
        }

        curr_spec->width = (uint32_t)width;
    } else {
        const int value = va_arg(list_struct->list, int);
        curr_spec->width = value >= 0 ? (uint32_t)value : 0;

        iter++;
    }

    if (*iter == '\0') {
        // If we have an incomplete spec, then we exit without writing
        // anything.

        return false;
    }

    *iter_out = iter;
    return true;
}

__optimize(3) static bool
parse_precision(struct printf_spec_info *const curr_spec,
                const char *iter,
                struct va_list_struct *const list_struct,
                const char **const iter_out)
{
    if (*iter != '.') {
        curr_spec->precision = -1;
        return true;
    }

    iter++;
    switch (*iter) {
        case '\0':
            return false;
        case '*':
            // Don't bother reading if we have an incomplete spec
            if (iter[1] == '\0') {
                return false;
            }

            curr_spec->precision = va_arg(list_struct->list, int);
            iter++;

            break;
        default: {
            curr_spec->precision = read_int_from_fmt_string(iter, &iter);
            if (curr_spec->precision == -1) {
                return false;
            }

            if (*iter == '\0') {
                return false;
            }

            break;
        }
    }

    *iter_out = iter;
    return true;
}

__optimize(3) static bool
parse_length(struct printf_spec_info *const curr_spec,
             const char *iter,
             const char **const iter_out,
             struct va_list_struct *const list_struct,
             uint64_t *const number_out,
             bool *const is_zero_out)
{
    switch (*iter) {
        case '\0':
            // If we get an incomplete spec, then we exit without writing
            // anything.
            return false;
        case 'h': {
            iter++;
            switch (*iter) {
                case '\0':
                    // If we get an incomplete spec, then we exit without
                    // writing anything.
                    return false;
                case 'h': {
                    const uint64_t number =
                        (uint64_t)(signed char)va_arg(list_struct->list, int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_sv = sv_create_length(iter - 2, 2);
                    iter++;
                    break;
                }
                default: {
                    const uint64_t number =
                        (uint64_t)(short int)va_arg(list_struct->list, int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_sv = sv_create_length(iter - 1, 1);
                    break;
                }
            }

            break;
        }
        case 'l':
            iter++;
            switch (*iter) {
                case '\0':
                    return false;
                case 'l': {
                    const uint64_t number =
                        (uint64_t)va_arg(list_struct->list, long long int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_sv = sv_create_length(iter - 2, 2);
                    iter++;

                    break;
                }
                default: {
                    const uint64_t number =
                        (uint64_t)va_arg(list_struct->list, long int);

                    *number_out = number;
                    *is_zero_out = number == 0;

                    curr_spec->length_sv = sv_create_length(iter - 1, 1);
                    break;
                }
            }

            break;
        case 'j': {
            const uint64_t number =
                (uint64_t)va_arg(list_struct->list, intmax_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_sv = sv_create_length(iter, 1);
            iter++;

            break;
        }
        case 'z': {
            const uint64_t number = (uint64_t)va_arg(list_struct->list, size_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_sv = sv_create_length(iter, 1);
            iter++;

            break;
        }
        case 't': {
            const uint64_t number =
                (uint64_t)va_arg(list_struct->list, ptrdiff_t);

            *number_out = number;
            *is_zero_out = number == 0;

            curr_spec->length_sv = sv_create_length(iter, 1);
            iter++;

            break;
        }
        default:
            curr_spec->length_sv = SV_EMPTY();
            return true;
    }

    *iter_out = iter;
    return true;
}

enum handle_spec_result {
    E_HANDLE_SPEC_OK,
    E_HANDLE_SPEC_REACHED_END,
    E_HANDLE_SPEC_CONTINUE
};

__optimize(3) static enum handle_spec_result
handle_spec(struct printf_spec_info *const curr_spec,
            char *const buffer,
            uint64_t number,
            struct va_list_struct *const list_struct,
            const uint64_t written_out,
            struct string_view *const parsed_out,
            bool *const is_zero_out,
            bool *const is_null_out)
{
    switch (curr_spec->spec) {
        case '\0':
            return E_HANDLE_SPEC_REACHED_END;
        case 'b':
            if (curr_spec->length_sv.length == 0) {
                number = (uint64_t)va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_2,
                                        buffer,
                                        NUM_TO_STR_OPTIONS_INIT());
            break;
        case 'B':
            if (curr_spec->length_sv.length == 0) {
                number = (uint64_t)va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_2,
                                        buffer,
                                        (struct num_to_str_options){
                                          .capitalize = true
                                        });
            break;
        case 'd':
        case 'i':
            if (curr_spec->length_sv.length == 0) {
                number = (uint64_t)va_arg(list_struct->list, int);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                signed_to_string_view((int64_t)number,
                                      NUMERIC_BASE_10,
                                      buffer,
                                      (struct num_to_str_options){
                                        .include_pos_sign =
                                            curr_spec->add_pos_sign,
                                      });
            break;
        case 'u':
            if (curr_spec->length_sv.length == 0) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_10,
                                        buffer,
                                        NUM_TO_STR_OPTIONS_INIT());
            break;
        case 'o':
            if (curr_spec->length_sv.length == 0) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = (number == 0);
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_8,
                                        buffer,
                                        NUM_TO_STR_OPTIONS_INIT());
            break;
        case 'x':
            if (curr_spec->length_sv.length == 0) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_16,
                                        buffer,
                                        NUM_TO_STR_OPTIONS_INIT());
            break;
        case 'X':
            if (curr_spec->length_sv.length == 0) {
                number = va_arg(list_struct->list, unsigned);
                *is_zero_out = number == 0;
            }

            *parsed_out =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_16,
                                        buffer,
                                        (struct num_to_str_options){
                                            .capitalize = true
                                        });
            break;
        case 'c':
            buffer[0] = (char)va_arg(list_struct->list, int);
            *parsed_out = sv_create_nocheck(buffer, 1);

            break;
        case 's': {
            const char *const str = va_arg(list_struct->list, const char *);
            if (str != NULL) {
                uint64_t length = 0;
                if (curr_spec->precision != -1) {
                    length = strnlen(str, (size_t)curr_spec->precision);
                } else {
                    length = strlen(str);
                }

                *parsed_out = sv_create_length(str, length);
            } else {
                *parsed_out = SV_STATIC("(null)");
                *is_null_out = true;
            }

            break;
        }
        case 'p': {
            const void *const arg = va_arg(list_struct->list, const void *);
            if (arg != NULL) {
                const struct num_to_str_options options = {
                    .capitalize = true,
                    .include_prefix = true
                };

                *parsed_out =
                    unsigned_to_string_view((uint64_t)arg,
                                            NUMERIC_BASE_16,
                                            buffer,
                                            options);
            } else {
                *parsed_out = SV_STATIC("(nil)");
                *is_null_out = true;
            }

            break;
        }
        case 'n':
            if (curr_spec->length_sv.length == 0) {
                *va_arg(list_struct->list, int *) = written_out;
                return E_HANDLE_SPEC_CONTINUE;
            }

            switch (*curr_spec->length_sv.begin) {
                case 'h':
                    // case 'hh'
                    if (curr_spec->length_sv.length == 2) {
                        if (curr_spec->length_sv.begin[1] == 'h') {
                            *va_arg(list_struct->list, signed char *) =
                                written_out;
                            return E_HANDLE_SPEC_CONTINUE;
                        }
                    } else if (curr_spec->length_sv.length == 1) {
                        *va_arg(list_struct->list, short int *) = written_out;
                        return E_HANDLE_SPEC_CONTINUE;
                    }

                    break;
                case 'l':
                    // case 'll'
                    if (curr_spec->length_sv.length == 2) {
                        if (curr_spec->length_sv.begin[1] == 'l') {
                            *va_arg(list_struct->list, signed char *) =
                                written_out;

                            return E_HANDLE_SPEC_CONTINUE;
                        }
                    } else if (curr_spec->length_sv.length == 1) {
                        *va_arg(list_struct->list, long int *) =
                            (long int)written_out;

                        return E_HANDLE_SPEC_CONTINUE;
                    }

                    break;
                case 'j':
                    *va_arg(list_struct->list, intmax_t *) =
                        (intmax_t)written_out;
                    return E_HANDLE_SPEC_CONTINUE;
                case 'z':
                    *va_arg(list_struct->list, size_t *) = written_out;
                    return E_HANDLE_SPEC_CONTINUE;
                case 't':
                    *va_arg(list_struct->list, ptrdiff_t *) =
                        (ptrdiff_t)written_out;
                    return E_HANDLE_SPEC_CONTINUE;
            }

            break;
        case '%':
            buffer[0] = '%';
            *parsed_out = sv_create_nocheck(buffer, 1);

            break;
        default:
            return E_HANDLE_SPEC_CONTINUE;
    }

    return E_HANDLE_SPEC_OK;
}

__optimize(3) static inline bool is_int_specifier(const char spec) {
    switch (spec) {
        case 'b':
        case 'B':
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            return true;
    }

    return false;
}

__optimize(3) static inline uint64_t
write_prefix_for_spec(struct printf_spec_info *const info,
                      const printf_write_char_callback_t write_ch_cb,
                      void *const ch_cb_info,
                      const printf_write_sv_callback_t write_sv_cb,
                      void *const sv_cb_info,
                      bool *const cont_out)
{
    uint64_t out = 0;
    if (!info->add_base_prefix) {
        return out;
    }

    switch (info->spec) {
        case 'b':
            out += write_sv_cb(info, sv_cb_info, SV_STATIC("0b"), cont_out);
            break;
        case 'B':
            out += write_sv_cb(info, sv_cb_info, SV_STATIC("0B"), cont_out);
            break;
        case 'o':
            out += write_ch_cb(info, ch_cb_info, '0', 1, cont_out);
            break;
        case 'x':
            out += write_sv_cb(info, sv_cb_info, SV_STATIC("0x"), cont_out);
            break;
        case 'X':
            out += write_sv_cb(info, sv_cb_info, SV_STATIC("0X"), cont_out);
            break;
    }

    return out;
}

__optimize(3) static uint64_t
pad_with_lead_zeros(struct printf_spec_info *const info,
                    struct string_view *const parsed,
                    const uint64_t zero_count,
                    const bool is_null,
                    const printf_write_char_callback_t write_char_cb,
                    void *const cb_info,
                    const printf_write_sv_callback_t write_sv_cb,
                    void *const write_sv_cb_info,
                    bool *const cont_out)
{
    uint64_t out = 0;
    if (!is_null) {
        out +=
            write_prefix_for_spec(info,
                                  write_char_cb,
                                  cb_info,
                                  write_sv_cb,
                                  write_sv_cb_info,
                                  cont_out);
    }

    if (zero_count == 0) {
        return out;
    }

    const char front = *parsed->begin;
    if (front == '+') {
        // We should only have pos signs when the spec requested it.
        out += write_char_cb(info, cb_info, '+', /*amount=*/1, cont_out);
        if (!cont_out) {
            return out;
        }

        *parsed = sv_drop_front(*parsed);
    } else if (front == '-') {
        out += write_char_cb(info, cb_info, '-', /*amount=*/1, cont_out);
        if (!cont_out) {
            return out;
        }

        *parsed = sv_drop_front(*parsed);
    }

    out += write_char_cb(info, cb_info, '0', zero_count, cont_out);
    if (!cont_out) {
        return out;
    }

    return out;
}

__optimize(3) static inline uint64_t
call_cb(struct printf_spec_info *const info,
        const struct string_view sv,
        const printf_write_char_callback_t write_char_cb,
        void *const char_cb_info,
        const printf_write_sv_callback_t write_sv_cb,
        void *const sv_cb_info,
        bool *const cont_out)
{
    uint64_t out = 0;
    if (sv.length == 0) {
        return out;
    }

    if (sv.length == 1) {
        const char ch = *sv.begin;
        return write_char_cb(info, char_cb_info, ch, /*amount=*/1, cont_out);
    }

    return write_sv_cb(info, sv_cb_info, sv, cont_out);
}

__optimize(3) uint64_t
parse_printf(const char *const fmt,
             const printf_write_char_callback_t write_char_cb,
             void *const write_char_cb_info,
             const printf_write_sv_callback_t write_sv_cb,
             void *const write_sv_cb_info,
             va_list list)
{
    struct va_list_struct list_struct = {0};
    va_copy(list_struct.list, list);

    // Add 2 for a int-prefix, and one for a sign.
    char buffer[MAX_CONVERT_CAP + 3];
    bzero(buffer, sizeof(buffer));

    struct printf_spec_info curr_spec = PRINTF_SPEC_INFO_INIT();
    const char *unformatted_start = fmt;

    uint64_t written_out = 0;
    bool should_continue = true;

    const char *iter = strchr(fmt, '%');
    for (; iter != NULL; iter = strchr(iter, '%')) {
        const struct string_view unformatted =
            sv_create_end(unformatted_start, iter);

        written_out +=
            call_cb(NULL,
                    unformatted,
                    write_char_cb,
                    write_char_cb_info,
                    write_sv_cb,
                    write_sv_cb_info,
                    &should_continue);

        if (!should_continue) {
            break;
        }

        iter++;
        if (*iter == '\0') {
            // If we only got a percent sign, then we don't print anything
            va_end(list_struct.list);
            return written_out;
        }

        // Format is %[flags][width][.precision][length]specifier
        if (!parse_flags(&curr_spec, iter, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            va_end(list_struct.list);
            return written_out;
        }

        if (!parse_width(&curr_spec, &list_struct, iter, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            va_end(list_struct.list);
            return written_out;
        }

        if (!parse_precision(&curr_spec, iter, &list_struct, &iter)) {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            va_end(list_struct.list);
            return written_out;
        }

        uint64_t number = 0;
        bool is_zero = false;

        if (!parse_length(&curr_spec,
                          iter,
                          &iter,
                          &list_struct,
                          &number,
                          &is_zero))
        {
            // If we have an incomplete spec, then we exit without writing
            // anything.
            va_end(list_struct.list);
            return written_out;
        }

        // Parse specifier
        struct string_view parsed = SV_EMPTY();
        bool is_null = false;

        curr_spec.spec = *iter;
        unformatted_start = iter + 1;

        const enum handle_spec_result handle_spec_result =
            handle_spec(&curr_spec,
                        buffer,
                        number,
                        &list_struct,
                        written_out,
                        &parsed,
                        &is_zero,
                        &is_null);

        switch (handle_spec_result) {
            case E_HANDLE_SPEC_OK:
                break;
            case E_HANDLE_SPEC_REACHED_END:
                va_end(list_struct.list);
                return written_out;
            case E_HANDLE_SPEC_CONTINUE:
                curr_spec = PRINTF_SPEC_INFO_INIT();
                continue;
        }

        /* Move past specifier */
        iter++;

        uint32_t padded_zero_count = 0;
        uint8_t parsed_length = parsed.length;

        // is_zero being true implies spec is an integer.
        // We don't write anything if we have a '0' and precision is 0.

        const bool should_write_parsed = !(is_zero && curr_spec.precision == 0);
        if (!should_write_parsed) {
            parsed_length = 0;
        } else if (curr_spec.add_base_prefix) {
            switch (curr_spec.spec) {
                case 'o':
                    parsed_length += 1;
                    break;
                case 'x':
                case 'X':
                    parsed_length += 2;
                    break;
            }
        }

        // If we're not wider than the specified width, we have to pad with
        // either spaces or zeroes.

        uint32_t space_pad_count = 0;
        if (is_int_specifier(curr_spec.spec)) {
            if (curr_spec.precision != -1) {
                // The case for the string-spec was already handled above
                // Total digit count doesn't include the sign/prefix.

                uint8_t total_digit_count = parsed.length;
                if (*parsed.begin == '-' || *parsed.begin == '+') {
                    total_digit_count -= 1;
                }

                if (total_digit_count < curr_spec.precision) {
                    padded_zero_count =
                        (uint32_t)curr_spec.precision - total_digit_count;

                    parsed_length += padded_zero_count;
                }
            }

            if (parsed_length != 0 && curr_spec.add_one_space_for_sign) {
                // Only add a sign if we have neither a '+' or '-'
                if (*parsed.begin != '+' && *parsed.begin != '-') {
                    space_pad_count += 1;
                }
            }
        }

        if (parsed_length < curr_spec.width) {
            const bool pad_with_zeros =
                curr_spec.leftpad_zeros &&
                is_int_specifier(curr_spec.spec) &&
                curr_spec.precision == -1 &&
                !curr_spec.left_justify; // Zeros are never left-justified

            if (pad_with_zeros) {
                // We're always resetting padded_zero_count if it was set before
                padded_zero_count = curr_spec.width - parsed_length;
            } else {
                space_pad_count += curr_spec.width - parsed_length;
            }
        }

        if (curr_spec.left_justify) {
            written_out +=
                pad_with_lead_zeros(&curr_spec,
                                    &parsed,
                                    padded_zero_count,
                                    is_null,
                                    write_char_cb,
                                    write_char_cb_info,
                                    write_sv_cb,
                                    write_sv_cb_info,
                                    &should_continue);

            if (!should_continue) {
                va_end(list_struct.list);
                return written_out;
            }

            if (should_write_parsed) {
                written_out +=
                    call_cb(&curr_spec,
                            parsed,
                            write_char_cb,
                            write_char_cb_info,
                            write_sv_cb,
                            write_sv_cb_info,
                            &should_continue);

                if (!should_continue) {
                    va_end(list_struct.list);
                    return written_out;
                }
            }

            if (space_pad_count != 0) {
                written_out +=
                    write_char_cb(&curr_spec,
                                  write_char_cb_info,
                                  ' ',
                                  space_pad_count,
                                  &should_continue);

                if (!should_continue) {
                    va_end(list_struct.list);
                    return written_out;
                }
            }
        } else {
            if (space_pad_count != 0) {
                written_out +=
                    write_char_cb(&curr_spec,
                                  write_char_cb_info,
                                  ' ',
                                  space_pad_count,
                                  &should_continue);

                if (!should_continue) {
                    va_end(list_struct.list);
                    return written_out;
                }
            }

            written_out +=
                pad_with_lead_zeros(&curr_spec,
                                    &parsed,
                                    padded_zero_count,
                                    is_null,
                                    write_char_cb,
                                    write_char_cb_info,
                                    write_sv_cb,
                                    write_sv_cb_info,
                                    &should_continue);

            if (!should_continue) {
                va_end(list_struct.list);
                return written_out;
            }

            if (should_write_parsed) {
                written_out +=
                    call_cb(&curr_spec,
                            parsed,
                            write_char_cb,
                            write_char_cb_info,
                            write_sv_cb,
                            write_sv_cb_info,
                            &should_continue);

                if (!should_continue) {
                    va_end(list_struct.list);
                    return written_out;
                }
            }
        }

        curr_spec = PRINTF_SPEC_INFO_INIT();
    }

    if (*unformatted_start != '\0') {
        const struct string_view unformatted =
            sv_create_length(unformatted_start, strlen(unformatted_start));

        curr_spec = PRINTF_SPEC_INFO_INIT();
        written_out +=
            call_cb(NULL,
                    unformatted,
                    write_char_cb,
                    write_char_cb_info,
                    write_sv_cb,
                    write_sv_cb_info,
                    &should_continue);
    }

    va_end(list_struct.list);
    return written_out;
}