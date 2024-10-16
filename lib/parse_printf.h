/*
 * lib/parse_printf.h
 * © suhas pai
 */

#pragma once

#include <stdarg.h>
#include "lib/adt/string_view.h"

struct printf_spec_info {
    bool add_one_space_for_sign : 1;
    bool left_justify : 1;
    bool add_pos_sign : 1;
    bool add_base_prefix : 1;
    bool leftpad_zeros : 1;

    char spec;
    uint32_t width;
    int precision; // -1 if no precision was provided

    struct string_view length_sv;
};

#define PRINTF_SPEC_INFO_INIT() \
    ((struct printf_spec_info) { \
        .add_one_space_for_sign = false, \
        .left_justify = false, \
        .add_pos_sign = false, \
        .leftpad_zeros = false, \
        .spec = '\0', \
        .width = 0, \
        .precision = 0, \
        .length_sv = SV_EMPTY() \
    })

/*
 * All callbacks should return length written-out.
 * should_continue_out is initialized to true.
 *
 * spec_info is NULL for callbacks to write unformatted strings.
 */

typedef uint32_t
(*printf_write_char_callback_t)(struct printf_spec_info *spec_info,
                                void *info,
                                char ch,
                                uint32_t times,
                                bool *should_continue_out);

typedef uint32_t
(*printf_write_sv_callback_t)(struct printf_spec_info *spec_info,
                              void *info,
                              struct string_view sv,
                              bool *should_continue_out);

uint32_t
parse_printf(const char *fmt,
             printf_write_char_callback_t write_char_cb,
             void *char_cb_info,
             printf_write_sv_callback_t write_sv_cb,
             void *sv_cb_info,
             va_list list);
