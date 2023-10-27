/*
 * src/lib/time.h
 * © suhas pai
 */

#include <limits.h>
#include "lib/assert.h"

#include "macros.h"
#include "time.h"

#define HAVE_HPUX_EXT 1
#define HAVE_SUNOS_EXT 1
#define HAVE_VMS_EXT 1

__optimize(3) uint8_t hour_12_to_24hour(const uint8_t hour, const bool is_pm) {
    // Use modulo to wrap hour 12 to hour 0
    return (is_pm) ? ((hour % 12) + 12) : (hour % 12);
}

__optimize(3) uint8_t hour_24_to_12hour(const uint8_t hour) {
    const uint8_t result = (hour % 12);
    return (result != 0) ? result : 12;
}

__optimize(3) bool hour_24_is_pm(const uint8_t hour) {
    return (hour >= 12);
}

__optimize(3)
uint8_t weekday_to_decimal_monday_one(const enum weekday weekday) {
    const uint8_t result =
        (weekday != WEEKDAY_SUNDAY) ?
            (uint8_t)weekday : 7; // Wrap WEEKDAY_SUNDAY from 0 to 7

    return result;
}

__optimize(3)
uint8_t month_get_day_count(const enum month month, const bool in_leap_year) {
    switch (month) {
        case MONTH_INVALID:
            verify_not_reached();
        case MONTH_FEBRUARY:
            return in_leap_year ? 29 : 28;
        case MONTH_JANUARY:
        case MONTH_MARCH:
        case MONTH_MAY:
        case MONTH_JULY:
        case MONTH_AUGUST:
        case MONTH_OCTOBER:
        case MONTH_DECEMBER:
            return 31;
        case MONTH_APRIL:
        case MONTH_JUNE:
        case MONTH_SEPTEMBER:
        case MONTH_NOVEMBER:
            return 30;
    }

    verify_not_reached();
}

__optimize(3) uint8_t
get_week_count_at_day(const enum weekday weekday,
                      const uint16_t days_since_jan_1,
                      const bool is_monday_first)
{
    // Based on implementation specified here:
    // https://github.com/arnoldrobbins/strftime/blob/master/strftime.c#L990-L1032

    const uint64_t delta =
        (is_monday_first ?
            ((weekday != WEEKDAY_SUNDAY) ? (WEEKDAY_COUNT + 1) : 1) :
            WEEKDAY_COUNT);

    return (days_since_jan_1 - (unsigned)weekday + delta) / 7;
}

__optimize(3) int month_to_tm_mon(const enum month month) {
    // Subtract one as tm_mon is actually "months since january", being
    // zero-indexed and having the range [0, 11]. Instead, we have [1, 12].

    assert(month != MONTH_INVALID);
    return (int)(month - 1);
}

__optimize(3) enum month tm_mon_to_month(const int tm_mon) {
    // Add one as tm_mon is actually "months since january", being zero-indexed
    // and having the range [0, 11]. Instead, we need [1, 12].

    return (enum month)(tm_mon + 1);
}

__optimize(3) bool year_is_leap_year(const uint64_t year) {
    // Year has to be divisible by 4 (first two lsb bits are zero) and either
    // the year is not divisible by 100 or the year is divisible by 400.

    return (
        ((year & 0b11) == 0) &&
        ((year % 100) != 0 || ((year % 400) == 0))
    );
}

__optimize(3) uint16_t year_get_day_count(const uint64_t year) {
    return (MIN_DAYS_IN_YEAR + (year_is_leap_year(year) ? 1 : 0));
}

__optimize(3) int year_to_tm_year(const uint64_t year) {
    assert(year <= INT_MAX);
    return ((int)year - 1900);
}

__optimize(3) struct tm tm_from_stamp(const uint64_t timestamp) {
    const int seconds_of_day = (int)seconds_mod_days(timestamp);
    const int minutes_of_day = seconds_to_minutes(seconds_of_day);
    const int hour_of_day = minutes_to_hours(minutes_of_day);

    const int seconds_of_minute = seconds_mod_minutes(seconds_of_day);
    const int minutes_of_hour = minutes_mod_hours(minutes_of_day);

    int day_of_year = seconds_to_days(timestamp);
    uint64_t year = 1970;

    while (day_of_year >= MIN_DAYS_IN_YEAR) {
        day_of_year -= year_get_day_count(year);
        year++;
    }

    const bool is_leap_year = year_is_leap_year(year);
    enum month month = MONTH_JANUARY;
    int day_of_month = day_of_year;

    while (day_of_month >= month_get_day_count(month, is_leap_year)) {
        day_of_month -= month_get_day_count(month, is_leap_year);
        month++;
    }

    return (struct tm){
        .tm_sec = seconds_of_minute,
        .tm_min = minutes_of_hour,
        .tm_hour = hour_of_day,
        .tm_mday = day_of_month + 1, // days is zero-indexed but tm_mday is not
        .tm_mon = month_to_tm_mon(month),
        .tm_year = year_to_tm_year(year),
        .tm_wday = day_of_month_to_weekday(year, month, day_of_month),
        .tm_yday = day_of_year,
        .tm_isdst = 0
    };
}

__optimize(3) uint64_t tm_year_to_year(const int tm_year) {
    return check_add_assert((uint64_t)tm_year, 1900);
}

__optimize(3) enum weekday weekday_prev(const enum weekday weekday) {
    return (weekday != WEEKDAY_SUNDAY) ? (weekday - 1) : WEEKDAY_SATURDAY;
}

__optimize(3) enum weekday weekday_next(const enum weekday weekday) {
    return get_to_within_size((uint64_t)weekday + 1, WEEKDAY_COUNT);
}

__optimize(3) struct string_view weekday_to_sv(const enum weekday day) {
    switch (day) {
#define WEEKDAY_CASE(name)                                                     \
    case VAR_CONCAT(WEEKDAY_, name):                                           \
        return VAR_CONCAT(SV_, name)

        WEEKDAY_CASE(SUNDAY);
        WEEKDAY_CASE(MONDAY);
        WEEKDAY_CASE(TUESDAY);
        WEEKDAY_CASE(WEDNESDAY);
        WEEKDAY_CASE(THURSDAY);
        WEEKDAY_CASE(FRIDAY);
        WEEKDAY_CASE(SATURDAY);

#undef WEEKDAY_CASE
        case WEEKDAY_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) struct string_view weekday_to_sv_upper(const enum weekday day) {
    switch (day) {
#define WEEKDAY_CASE(name)                                                     \
    case VAR_CONCAT(WEEKDAY_, name):                                           \
        return VAR_CONCAT_3(SV_, name, _UPPER)

        WEEKDAY_CASE(SUNDAY);
        WEEKDAY_CASE(MONDAY);
        WEEKDAY_CASE(TUESDAY);
        WEEKDAY_CASE(WEDNESDAY);
        WEEKDAY_CASE(THURSDAY);
        WEEKDAY_CASE(FRIDAY);
        WEEKDAY_CASE(SATURDAY);

#undef WEEKDAY_CASE
        case WEEKDAY_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) struct string_view month_to_sv(const enum month month) {
    switch (month) {
#define MONTH_CASE(name)                                                       \
    case VAR_CONCAT(MONTH_, name):                                             \
        return VAR_CONCAT(SV_, name)

        MONTH_CASE(JANUARY);
        MONTH_CASE(FEBRUARY);
        MONTH_CASE(MARCH);
        MONTH_CASE(APRIL);
        MONTH_CASE(MAY);
        MONTH_CASE(JUNE);
        MONTH_CASE(JULY);
        MONTH_CASE(AUGUST);
        MONTH_CASE(SEPTEMBER);
        MONTH_CASE(OCTOBER);
        MONTH_CASE(NOVEMBER);
        MONTH_CASE(DECEMBER);

#undef MONTH_CASE
        case MONTH_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) struct string_view month_to_sv_upper(const enum month month) {
    switch (month) {
#define MONTH_CASE(name)                                                       \
    case VAR_CONCAT(MONTH_, name):                                             \
        return VAR_CONCAT_3(SV_, name, _UPPER)

        MONTH_CASE(JANUARY);
        MONTH_CASE(FEBRUARY);
        MONTH_CASE(MARCH);
        MONTH_CASE(APRIL);
        MONTH_CASE(MAY);
        MONTH_CASE(JUNE);
        MONTH_CASE(JULY);
        MONTH_CASE(AUGUST);
        MONTH_CASE(SEPTEMBER);
        MONTH_CASE(OCTOBER);
        MONTH_CASE(NOVEMBER);
        MONTH_CASE(DECEMBER);

#undef MONTH_CASE
        case MONTH_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) struct string_view weekday_to_sv_abbrev(const enum weekday day) {
    switch (day) {
#define WEEKDAY_CASE(name)                                                     \
    case VAR_CONCAT(WEEKDAY_, name):                                           \
        return VAR_CONCAT_3(SV_, name, _ABBREV)

        WEEKDAY_CASE(SUNDAY);
        WEEKDAY_CASE(MONDAY);
        WEEKDAY_CASE(TUESDAY);
        WEEKDAY_CASE(WEDNESDAY);
        WEEKDAY_CASE(THURSDAY);
        WEEKDAY_CASE(FRIDAY);
        WEEKDAY_CASE(SATURDAY);

#undef WEEKDAY_CASE
        case WEEKDAY_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3)
struct string_view weekday_to_sv_abbrev_upper(const enum weekday day) {
    switch (day) {
#define WEEKDAY_CASE(name)                                                     \
    case VAR_CONCAT(WEEKDAY_, name):                                           \
        return VAR_CONCAT_3(SV_, name, _ABBREV_UPPER)

        WEEKDAY_CASE(SUNDAY);
        WEEKDAY_CASE(MONDAY);
        WEEKDAY_CASE(TUESDAY);
        WEEKDAY_CASE(WEDNESDAY);
        WEEKDAY_CASE(THURSDAY);
        WEEKDAY_CASE(FRIDAY);
        WEEKDAY_CASE(SATURDAY);

#undef WEEKDAY_CASE
        case WEEKDAY_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) struct string_view month_to_sv_abbrev(const enum month month) {
    switch (month) {
#define MONTH_CASE(name)                                                       \
    case VAR_CONCAT(MONTH_, name):                                             \
        return VAR_CONCAT_3(SV_, name, _ABBREV)

        MONTH_CASE(JANUARY);
        MONTH_CASE(FEBRUARY);
        MONTH_CASE(MARCH);
        MONTH_CASE(APRIL);
        MONTH_CASE(MAY);
        MONTH_CASE(JUNE);
        MONTH_CASE(JULY);
        MONTH_CASE(AUGUST);
        MONTH_CASE(SEPTEMBER);
        MONTH_CASE(OCTOBER);
        MONTH_CASE(NOVEMBER);
        MONTH_CASE(DECEMBER);

#undef MONTH_CASE
        case MONTH_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3)
struct string_view month_to_sv_abbrev_upper(const enum month month) {
    switch (month) {
#define MONTH_CASE(name)                                                       \
    case VAR_CONCAT(MONTH_, name):                                             \
        return VAR_CONCAT_3(SV_, name, _ABBREV_UPPER)

        MONTH_CASE(JANUARY);
        MONTH_CASE(FEBRUARY);
        MONTH_CASE(MARCH);
        MONTH_CASE(APRIL);
        MONTH_CASE(MAY);
        MONTH_CASE(JUNE);
        MONTH_CASE(JULY);
        MONTH_CASE(AUGUST);
        MONTH_CASE(SEPTEMBER);
        MONTH_CASE(OCTOBER);
        MONTH_CASE(NOVEMBER);
        MONTH_CASE(DECEMBER);

#undef MONTH_CASE
        case MONTH_INVALID:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) enum weekday sv_to_weekday(const struct string_view sv) {
#define RETURN_IF_EQUAL(name)                                                 \
    do {                                                                       \
        if (sv_equals(sv, VAR_CONCAT(SV_, name))) {                            \
            return VAR_CONCAT(WEEKDAY_, name);                                 \
        }                                                                      \
    } while (false)

    RETURN_IF_EQUAL(SUNDAY);
    RETURN_IF_EQUAL(MONDAY);
    RETURN_IF_EQUAL(TUESDAY);
    RETURN_IF_EQUAL(WEDNESDAY);
    RETURN_IF_EQUAL(THURSDAY);
    RETURN_IF_EQUAL(FRIDAY);
    RETURN_IF_EQUAL(SATURDAY);

#undef RETURN_IF_EQUAL
    return WEEKDAY_INVALID;
}

__optimize(3) enum month sv_to_month(const struct string_view sv) {
#define RETURN_IF_EQUAL(name)                                                 \
    do {                                                                       \
        if (sv_equals(sv, VAR_CONCAT(SV_, name))) {                            \
            return VAR_CONCAT(MONTH_, name);                                   \
        }                                                                      \
    } while (false)

    RETURN_IF_EQUAL(JANUARY);
    RETURN_IF_EQUAL(FEBRUARY);
    RETURN_IF_EQUAL(MARCH);
    RETURN_IF_EQUAL(APRIL);
    RETURN_IF_EQUAL(MAY);
    RETURN_IF_EQUAL(JUNE);
    RETURN_IF_EQUAL(JULY);
    RETURN_IF_EQUAL(AUGUST);
    RETURN_IF_EQUAL(SEPTEMBER);
    RETURN_IF_EQUAL(OCTOBER);
    RETURN_IF_EQUAL(NOVEMBER);
    RETURN_IF_EQUAL(DECEMBER);

#undef RETURN_IF_EQUAL
    return MONTH_INVALID;
}

__optimize(3) enum weekday sv_abbrev_to_weekday(const struct string_view sv) {
#define RETURN_IF_EQUAL(name)                                                 \
    do {                                                                       \
        if (sv_equals(sv, VAR_CONCAT_3(SV_, name, _ABBREV))) {                 \
            return VAR_CONCAT(WEEKDAY_, name);                                 \
        }                                                                      \
    } while (false)

    RETURN_IF_EQUAL(SUNDAY);
    RETURN_IF_EQUAL(MONDAY);
    RETURN_IF_EQUAL(TUESDAY);
    RETURN_IF_EQUAL(WEDNESDAY);
    RETURN_IF_EQUAL(THURSDAY);
    RETURN_IF_EQUAL(FRIDAY);
    RETURN_IF_EQUAL(SATURDAY);

#undef RETURN_IF_EQUAL
    return WEEKDAY_INVALID;
}

__optimize(3) enum month sv_abbrev_to_month(const struct string_view sv) {
#define RETURN_IF_EQUAL(name)                                                 \
    do {                                                                       \
        if (sv_equals(sv, VAR_CONCAT_3(SV_, name, _ABBREV))) {                 \
            return VAR_CONCAT(MONTH_, name);                                   \
        }                                                                      \
    } while (false)

    RETURN_IF_EQUAL(JANUARY);
    RETURN_IF_EQUAL(FEBRUARY);
    RETURN_IF_EQUAL(MARCH);
    RETURN_IF_EQUAL(APRIL);
    RETURN_IF_EQUAL(MAY);
    RETURN_IF_EQUAL(JUNE);
    RETURN_IF_EQUAL(JULY);
    RETURN_IF_EQUAL(AUGUST);
    RETURN_IF_EQUAL(SEPTEMBER);
    RETURN_IF_EQUAL(OCTOBER);
    RETURN_IF_EQUAL(NOVEMBER);
    RETURN_IF_EQUAL(DECEMBER);

#undef RETURN_IF_EQUAL
    return MONTH_INVALID;
}

__optimize(3) enum weekday
day_of_month_to_weekday(uint64_t year,
                        const enum month month,
                        const uint8_t day)
{
    // Algorithm from https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html
    if (month < MONTH_MARCH) {
        year -= 1;
    }

    return (enum weekday)(
        (year + (year / 4) - (year / 100) + (year / 400) +
         (uint64_t)"bed=pen+mad."[(uint64_t)month - 1] + day) % 7
    );
}

__optimize(3) uint16_t
day_of_month_to_day_of_year(const uint8_t day_in_month,
                            const enum month month,
                            const bool in_leap_year)
{
    assert(month_is_valid(month));

    // Store the days preceding a month at index `(uint8_t)(month - 1)`
    static const uint16_t lookup_table[MONTH_COUNT] = {
        0,   // January
        31,  // February
        59,  // March
        90,  // April
        120, // May
        151, // June
        181, // July
        212, // August
        243, // September
        273, // October
        304, // November
        334  // December
    };

    // February has an extra day on leap years, which wasn't accounted for in
    // the lookup table.

    uint16_t days_preceding_month = lookup_table[(uint8_t)month - 1];
    if (in_leap_year) {
        // Each month after February has to account for the leap day in February
        // in leap years by adjusting their days-preceding entry.

        if (month > MONTH_FEBRUARY) {
            days_preceding_month += 1;
        }
    }

    return days_preceding_month + day_in_month;
}

__optimize(3) uint8_t
iso_8601_get_week_number(const enum weekday weekday,
                         const enum month month,
                         const uint64_t year,
                         const uint8_t day_in_month,
                         const uint16_t days_since_jan_1)
{
    assert(weekday_is_valid(weekday));
    assert(month_is_valid(month));

    enum weekday jan_1_weekday = (weekday - (days_since_jan_1 % 7));
    if (jan_1_weekday < 0) {
        jan_1_weekday += 7;
    }

    // Get week number, Monday as the first day of week
    uint8_t week_number =
        get_week_count_at_day(weekday,
                              days_since_jan_1,
                              /*is_monday_first=*/true);

    // According to the ISO 8601 format, January 1 on days Monday through
    // Thursday are in week 1. Otherwise January 1 is on the last week of the
    // previous year, which counts in this year as week 0.

    switch (jan_1_weekday) {
        case WEEKDAY_INVALID:
            verify_not_reached();
        case WEEKDAY_MONDAY:
            break;
        case WEEKDAY_TUESDAY:
        case WEEKDAY_WEDNESDAY:
        case WEEKDAY_THURSDAY:
            // Fix the calculation from the formula above
            week_number += 1;
            break;
        case WEEKDAY_FRIDAY:
        case WEEKDAY_SATURDAY:
        case WEEKDAY_SUNDAY:
            if (week_number != 0) {
                break;
            }

            // To figure this out, recursively call ourself for december 31's
            // week number.

            const enum weekday dec_31_weekday = weekday_prev(weekday);

            // The count of days preceding december 31 is that year's day count
            // minus one for december 31.

            const uint64_t days_preceding_dec_31 =
                year_get_day_count(year - 1) - 1;

            week_number =
                iso_8601_get_week_number(dec_31_weekday,
                                         MONTH_DECEMBER,
                                         year - 1,
                                         31,
                                         days_preceding_dec_31);
            break;
    }

    if (month == MONTH_DECEMBER) {
        /*
         * Handle situations where the last week of the previous year is in this
         * year's first week.
         *
         * This happens in one of the following situations:
         *   (a) M   T  W
         *       29 30 31
         *   (b) M  T  W
         *       30 31 1
         *   (c) M  T W
         *       31 1 2
         */

        switch (weekday) {
            case WEEKDAY_INVALID:
                verify_not_reached();
            case WEEKDAY_MONDAY:
                if (day_in_month >= 29 && day_in_month <= 31) {
                    week_number = 1;
                }

                break;
            case WEEKDAY_TUESDAY:
                if (day_in_month >= 30 && day_in_month <= 31) {
                    week_number = 1;
                }

                break;
            case WEEKDAY_WEDNESDAY:
                if (day_in_month == 31) {
                    week_number = 1;
                }

                break;
            case WEEKDAY_THURSDAY:
            case WEEKDAY_FRIDAY:
            case WEEKDAY_SATURDAY:
            case WEEKDAY_SUNDAY:
                break;
        }
    }

    return week_number;
}