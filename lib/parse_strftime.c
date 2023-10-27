/*
 * lib/parse_strftime.c
 * © suhas pai
 */

#include "convert.h"
#include "parse_strftime.h"

#define HAVE_SUNOS_EXT 1
#define HAVE_VMS_EXT 1

static inline struct strftime_modifiers
parse_strftime_mods(const char *const iter, const char **const iter_out) {
    struct strftime_modifiers mods = {0};
    switch (*iter) {
        case 'E':
            mods.locale_alt_repr = true;
            break;
        case 'O':
            mods.locale_alt_numeric = true;
            break;
        case '-':
            mods.pad_spaces_to_number = true;
            break;
        case '_':
            mods.dont_pad_number = true;
            break;
        case '0':
            mods.pad_zeros = true;
            break;
        case '^':
            mods.capitalize_letters = true;
            break;
        case '#':
            mods.swap_letter_case = true;
            break;
        default:
            return mods;
    }

    *iter_out = iter + 1;
    return mods;
}

static bool
handle_strftime_spec(const struct strftime_spec_info *const spec_info,
                     const parse_strftime_sv_callback sv_cb,
                     void *const sv_cb_info,
                     const struct tm *const tm,
                     uint64_t *const written_out,
                     bool *const should_cont_out)
{
#define CALL_CALLBACK(sv) \
    do {                                                                       \
        *written_out += sv_cb(spec_info, sv_cb_info, (sv), should_cont_out);   \
        if (!*should_cont_out) {                                               \
            goto done;                                                         \
        }                                                                      \
    } while (false)

#define RECURSIVE_CALL_FOR_SPEC(other_spec) \
    do {                                                                       \
        const struct strftime_spec_info other_spec_info = {                    \
            .spec = other_spec,                                                \
            .mods = spec_info->mods                                            \
        };                                                                     \
        const bool valid_spec =                                                \
            handle_strftime_spec(&other_spec_info,                             \
                                 sv_cb,                                        \
                                 sv_cb_info,                                   \
                                 tm,                                           \
                                 written_out,                                  \
                                 should_cont_out);                             \
        assert(valid_spec);                                                    \
        if (!*should_cont_out) {                                               \
            goto done;                                                         \
        }                                                                      \
    } while (false)

    /*
     * `conv_buffer` has enough space to fit the max 64-bit unsigned number, and
     * since our sv is at most 2 characters, we have enough space for a zero.
     */

#define PAD_WITH_CHAR_IF_NECESSARY(sv, ch, expected_len) \
    do {                                                                       \
        const struct strftime_modifiers mods = spec_info->mods;                \
        if (!mods.dont_pad_number) {                                           \
            char __pad_ch__ = ch;                                              \
            if (mods.pad_spaces_to_number) {                                   \
                __pad_ch__ = ' ';                                              \
            } else if (mods.pad_zeros) {                                       \
                __pad_ch__ = '0';                                              \
            }                                                                  \
            char *__sv_begin__ = sv_get_begin_mut(sv);                         \
            const char __sv_length__ = sv.length;                              \
            if (__sv_length__ < expected_len) {                                \
                const char *const __sv_end__ = sv_get_end(sv);                 \
                for (uint8_t __i__ = __sv_length__;                            \
                     __i__ != expected_len;                                    \
                     __i__++)                                                  \
                {                                                              \
                    __sv_begin__[-1] = __pad_ch__;                             \
                    __sv_begin__ -= 1;                                         \
                }                                                              \
                sv = sv_create_end(__sv_begin__, __sv_end__);                  \
            }                                                                  \
        }                                                                      \
    } while (false)

    // Simple macro to use the `_upper` function when we need to.
#define GET_SV_FROM_FUNC(func, ...) \
    (spec_info->mods.capitalize_letters ?                                      \
        VAR_CONCAT(func, _upper)(__VA_ARGS__) : func(__VA_ARGS__))             \

    char conv_buffer[MAX_CONVERT_CAP];
    bzero(conv_buffer, sizeof(conv_buffer));

    switch (spec_info->spec) {
        case 'a': {
            const enum weekday weekday = (enum weekday)tm->tm_wday;
            if (weekday_is_valid(weekday)) {
                CALL_CALLBACK(GET_SV_FROM_FUNC(weekday_to_sv_abbrev, weekday));
            } else {
                CALL_CALLBACK(SV_STATIC("?"));
            }

            break;
        }
        case 'A': { // Full weekday name
            const enum weekday weekday = (enum weekday)tm->tm_wday;
            if (weekday_is_valid(weekday)) {
                CALL_CALLBACK(GET_SV_FROM_FUNC(weekday_to_sv, weekday));
            } else {
                CALL_CALLBACK(SV_STATIC("?"));
            }

            break;
        }
        case 'b':
        case 'h': { // weekday name abbreviated
            const enum month month = tm_mon_to_month(tm->tm_mon);
            if (month_is_valid(month)) {
                CALL_CALLBACK(GET_SV_FROM_FUNC(month_to_sv_abbrev, month));
            } else {
                CALL_CALLBACK(SV_STATIC("?"));
            }

            break;
        }
        case 'B': { // Full Month name
            const enum month month = tm_mon_to_month(tm->tm_mon);
            if (month_is_valid(month)) {
                CALL_CALLBACK(GET_SV_FROM_FUNC(month_to_sv, month));
            } else {
                CALL_CALLBACK(SV_STATIC("?"));
            }

            break;
        }
        case 'c': // Equivalent to "%a %b %e %T %Y"
            RECURSIVE_CALL_FOR_SPEC('a');
            CALL_CALLBACK(SV_STATIC(" "));

            RECURSIVE_CALL_FOR_SPEC('b');
            CALL_CALLBACK(SV_STATIC(" "));

            RECURSIVE_CALL_FOR_SPEC('d');
            CALL_CALLBACK(SV_STATIC(" "));

            RECURSIVE_CALL_FOR_SPEC('T');
            CALL_CALLBACK(SV_STATIC(" "));

            RECURSIVE_CALL_FOR_SPEC('Y');
            break;
#if defined(HAVE_HPUX_EXT)
        /*
         * TODO: 'N' requires locale support. For now we just print out the
         * century
         */
        case 'N': // Emperor/Era name
#endif /* defined(HAVE_HPUX_EXT) */
        case 'C': { // Year divided by 100 (Last two digits)
            const uint64_t year = tm_year_to_year(tm->tm_year) / 100;
            const struct string_view sv =
                unsigned_to_string_view(year,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            CALL_CALLBACK(sv);
            break;
        }
        case 'd': { // Day of month, zero-padded
            struct string_view sv =
                unsigned_to_string_view((uint64_t)tm->tm_mday,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', 2);
            CALL_CALLBACK(sv);

            break;
        }
        case 'D': // Equivalent to %m/%d/%y
            RECURSIVE_CALL_FOR_SPEC('m');
            CALL_CALLBACK(SV_STATIC("/"));

            RECURSIVE_CALL_FOR_SPEC('d');
            CALL_CALLBACK(SV_STATIC("/"));

            RECURSIVE_CALL_FOR_SPEC('y');
            break;
        case 'e': { // Day of month, space-padded
            struct string_view sv =
                unsigned_to_string_view((uint64_t)tm->tm_mday,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, ' ', MAX_DAY_OF_MONTH_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'F': // Equivalent to %Y-%m-%d
            RECURSIVE_CALL_FOR_SPEC('Y');
            CALL_CALLBACK(SV_STATIC("-"));

            RECURSIVE_CALL_FOR_SPEC('m');
            CALL_CALLBACK(SV_STATIC("-"));

            RECURSIVE_CALL_FOR_SPEC('d');
            break;
        case 'g': // ISO 8601 Week-based year, last two digits (00-99)
        case 'G': { // ISO 8601 Week-based year
            const enum month month = tm_mon_to_month(tm->tm_mon);
            uint64_t year = tm_year_to_year(tm->tm_year);
            const uint8_t week_number =
                iso_8601_get_week_number((enum weekday)tm->tm_wday,
                                         month,
                                         year,
                                         (uint8_t)tm->tm_mday,
                                         (uint16_t)tm->tm_yday);

            if (month == MONTH_DECEMBER && week_number == 1) {
                /*
                 * If we're in December, but the week-number (of the year) is 1,
                 * then we're actually in the next year
                 */

                year += 1;
            } else if (month == MONTH_JANUARY && week_number >= 52) {
                /*
                 * If we're in January, but the week-number (of the year) is 52
                 * or higher, then we're in the previous year.
                 */

                year -= 1;
            }

            struct string_view sv = SV_EMPTY();
            if (spec_info->spec == 'g') {
                sv =
                    unsigned_to_string_view(year % 100,
                                            NUMERIC_BASE_10,
                                            conv_buffer,
                                            NUM_TO_STR_OPTIONS_INIT());

                PAD_WITH_CHAR_IF_NECESSARY(sv, '0', 2);
            } else {
                sv =
                    unsigned_to_string_view(year,
                                            NUMERIC_BASE_10,
                                            conv_buffer,
                                            NUM_TO_STR_OPTIONS_INIT());
            }

            CALL_CALLBACK(sv);
            break;
        }
        case 'H': { // Hour in 24-hour format
            struct string_view sv =
                unsigned_to_string_view((uint8_t)tm->tm_hour,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_HOUR_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'I': { // Hour in 12-hour format
            const uint64_t hour = hour_24_to_12hour((uint8_t)tm->tm_hour);
            struct string_view sv =
                unsigned_to_string_view(hour,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_HOUR_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'j': { // Day of the year
            /*
             * Add one since tm_yday is days since january 1st, while day of the
             * year is expected to start at 1.
             */

            struct string_view sv =
                unsigned_to_string_view((uint16_t)tm->tm_yday + 1,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_YEAR_DAY_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'm': { // Month as decimal number
            struct string_view sv =
                unsigned_to_string_view(tm_mon_to_month(tm->tm_mon),
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_MONTH_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'M': { // Minute
            struct string_view sv =
                unsigned_to_string_view((uint16_t)tm->tm_min,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_MINUTE_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'n': // new-line
            CALL_CALLBACK(SV_STATIC("\n"));
            break;
        case 'p': // AM or PM
            CALL_CALLBACK(
                hour_24_is_pm((uint8_t)tm->tm_hour) ?
                    SV_STATIC("PM") : SV_STATIC("AM"));
            break;
        case 'r': // Same as "%I:%M:%S %p"
            RECURSIVE_CALL_FOR_SPEC('I');
            CALL_CALLBACK(SV_STATIC(":"));

            RECURSIVE_CALL_FOR_SPEC('M');
            CALL_CALLBACK(SV_STATIC(":"));

            RECURSIVE_CALL_FOR_SPEC('S');
            CALL_CALLBACK(SV_STATIC(" "));

            RECURSIVE_CALL_FOR_SPEC('p');
            break;
        case 'R': // Same as "%H:%M"
            RECURSIVE_CALL_FOR_SPEC('H');
            CALL_CALLBACK(SV_STATIC(":"));
            RECURSIVE_CALL_FOR_SPEC('M');

            break;
        case 'S': { // Second
            struct string_view sv =
                unsigned_to_string_view((uint64_t)tm->tm_sec,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_SECOND_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 't': // Horizontal-Tab
            CALL_CALLBACK(SV_STATIC("\t"));
            break;
        case 'T':
        case 'X': // Same as "%H:%M:%S"
            RECURSIVE_CALL_FOR_SPEC('H');
            CALL_CALLBACK(SV_STATIC(":"));

            RECURSIVE_CALL_FOR_SPEC('M');
            CALL_CALLBACK(SV_STATIC(":"));

            RECURSIVE_CALL_FOR_SPEC('S');
            break;
        case 'u': {
            // ISO 8601: Weekday as decimal number. Monday = 1, Range = [0,7]
            const uint8_t number =
                weekday_to_decimal_monday_one((enum weekday)tm->tm_wday);
            struct string_view sv =
                unsigned_to_string_view(number,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_WEEKDAY_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'U': { // Week of year where Sunday is first day
            const uint64_t week_number =
                get_week_count_at_day((enum weekday)tm->tm_wday,
                                      /*days_since_jan_1=*/tm->tm_yday,
                                      /*is_monday_first=*/false);
            struct string_view sv =
                unsigned_to_string_view(week_number,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_WEEK_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'V': { // Week of year where Sunday is first day (iso 8601)
            const uint64_t week_number =
                iso_8601_get_week_number((enum weekday)tm->tm_wday,
                                         tm_mon_to_month(tm->tm_mon),
                                         tm_year_to_year(tm->tm_year),
                                         (uint8_t)tm->tm_mday,
                                         (uint16_t)tm->tm_yday);
            struct string_view sv =
                unsigned_to_string_view(week_number,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_WEEK_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'w': { // Weekday as decimal number. Sunday = 0, Range = [0, 6]
            struct string_view sv =
                unsigned_to_string_view((uint8_t)tm->tm_wday,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_WEEKDAY_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'W': { // Week of year where Monday is first day of the week
            const uint64_t week_number =
                get_week_count_at_day((enum weekday)tm->tm_wday,
                                      tm->tm_yday,
                                      /*is_monday_first=*/true);
            struct string_view sv =
                unsigned_to_string_view(week_number,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', MAX_WEEK_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'x': // Same as "%m/%d/%y"
            RECURSIVE_CALL_FOR_SPEC('m');
            CALL_CALLBACK(SV_STATIC("/"));

            RECURSIVE_CALL_FOR_SPEC('d');
            CALL_CALLBACK(SV_STATIC("/"));

            RECURSIVE_CALL_FOR_SPEC('y');
            break;
        case 'y': { // Year without century (Last two digits)
            const uint64_t last_two_digits = tm_year_to_year(tm->tm_year) % 100;
            struct string_view sv =
                unsigned_to_string_view(last_two_digits,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, '0', 2);
            CALL_CALLBACK(sv);

            break;
        }
        case 'Y': { // Year
            struct string_view sv =
                unsigned_to_string_view(tm_year_to_year(tm->tm_year),
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            CALL_CALLBACK(sv);
            break;
        }
#if HAVE_SUNOS_EXT
        case 'k': { // 24-hour format with space-pad
            struct string_view sv =
                unsigned_to_string_view((uint64_t)tm->tm_hour,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, ' ', MAX_HOUR_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
        case 'l': { // 12-hour format with space-pad
            const uint8_t hour = (uint8_t)hour_24_to_12hour(tm->tm_hour);
            struct string_view sv =
                unsigned_to_string_view(hour,
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(sv, ' ', MAX_HOUR_NUMBER_LENGTH);
            CALL_CALLBACK(sv);

            break;
        }
#endif /* HAVE_SUNOS_EXT */
#if HAVE_VMS_EXT
        case 'v': { // date as dd-bbb-YYYY
            RECURSIVE_CALL_FOR_SPEC('d');
            CALL_CALLBACK(SV_STATIC("-"));

            /*
             * We can't call 'm' specifier here because month has to be padded
             * by a space 3 times
             */

            struct string_view month_sv =
                unsigned_to_string_view(tm_mon_to_month(tm->tm_mon),
                                        NUMERIC_BASE_10,
                                        conv_buffer,
                                        NUM_TO_STR_OPTIONS_INIT());

            PAD_WITH_CHAR_IF_NECESSARY(month_sv, ' ', 3);
            CALL_CALLBACK(month_sv);

            RECURSIVE_CALL_FOR_SPEC('Y');
            break;
        }
#endif /* defined(HAVE_VMS_EXT) */
        // We don't support %+ as it requires detecting timezones
        // 'z' not supported (yet).
        case '%':
            CALL_CALLBACK(SV_STATIC("%"));
            break;
        default:
            return false;
    }

done:
    return true;
}

uint64_t
parse_strftime_format(const parse_strftime_sv_callback sv_cb,
                      void *const sv_cb_info,
                      const char *const format,
                      const struct tm *const tm)
{
    const char *unformat_buffer_ptr = format;
    bool should_continue = true;

    uint64_t written_out = 0;

    struct strftime_spec_info spec_info;
    const char *iter = strchr(format, '%');

    for (; iter != NULL; iter = strchr(iter, '%')) {
        spec_info = STRFTIME_SPEC_INFO_INIT();
        const struct string_view unformat_buffer_sv =
            sv_create_end(unformat_buffer_ptr, iter);

        if (unformat_buffer_sv.length != 0) {
            written_out +=
                sv_cb(NULL, sv_cb_info, unformat_buffer_sv, &should_continue);

            if (!should_continue) {
                break;
            }
        }

        /*
         * Keep a pointer to the start of the format, because if this
         * format-specifier is invalid, we'll need to treat it as an ordinary
         * unformatted string.
         */

        const char *orig_iter = iter;
        bool v_continue = true;

        // Skip past the '%' char.
        iter++;

        spec_info.mods = parse_strftime_mods(iter, &iter);
        spec_info.spec = *iter;

        const bool valid_spec =
            handle_strftime_spec(&spec_info,
                                 sv_cb,
                                 sv_cb_info,
                                 tm,
                                 &written_out,
                                 &v_continue);

        if (!v_continue) {
            break;
        }

        // Reset the unformat-buffer.
        iter++;
        if (valid_spec) {
            unformat_buffer_ptr = iter;
        } else {
            unformat_buffer_ptr = orig_iter;
        }
    }

    if (*unformat_buffer_ptr != '\0') {
        const struct string_view unformat_buffer_sv =
            sv_create_length(unformat_buffer_ptr, strlen(unformat_buffer_ptr));

        spec_info = STRFTIME_SPEC_INFO_INIT();
        written_out +=
            sv_cb(NULL, sv_cb_info, unformat_buffer_sv, &should_continue);
    }

    return 0;
}