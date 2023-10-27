/*
 * lib/parse_strftime.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "time.h"

struct strftime_modifiers {
    bool locale_alt_repr : 1;
    bool locale_alt_numeric : 1;
    bool pad_spaces_to_number : 1;
    bool dont_pad_number : 1;
    bool pad_zeros : 1;
    bool capitalize_letters : 1;
    bool swap_letter_case : 1;
};

#define STRFTIME_MODIFIERS_INIT() \
    ((struct strftime_modifiers) {\
        .locale_alt_repr = false, \
        .locale_alt_numeric = false, \
        .pad_spaces_to_number = false, \
        .dont_pad_number = false, \
        .pad_zeros = false, \
        .capitalize_letters = false, \
        .swap_letter_case = false \
    })

struct strftime_spec_info {
    char spec;
    struct strftime_modifiers mods;
};

#define STRFTIME_SPEC_INFO_INIT() \
    ((struct strftime_spec_info){ \
        .spec = '\0', \
        .mods = STRFTIME_MODIFIERS_INIT() \
    })

typedef uint64_t
(*parse_strftime_sv_callback)(const struct strftime_spec_info *spec_info,
                              void *cb_info,
                              struct string_view sv,
                              bool *should_continue_out);

uint64_t
parse_strftime_format(parse_strftime_sv_callback sv_cb,
                      void *sv_cb_info,
                      const char *format,
                      const struct tm *tm);
