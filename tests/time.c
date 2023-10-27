/*
 * tests/time.c
 * © suhas pai
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "lib/strftime.h"
#include "lib/time.h"

static struct tm *get_time_now() {
    time_t ts = 0;
    time(&ts);

    return localtime(&ts);
}

static void test_match(const char *const fmt) {
    const struct tm *const tm = get_time_now();

    char buffer[1024] = {0};
    time_format_to_string_buffer(buffer, sizeof(buffer), fmt, tm);

    char strftime_buffer[sizeof(buffer)] = {0};
    strftime(strftime_buffer, sizeof(strftime_buffer), fmt, tm);

    if (strcmp(buffer, strftime_buffer) != 0) {
        printf("\tFAILURE: Spec(%s): \"%s\" vs \"%s\"\n",
               fmt, buffer, strftime_buffer);
    }
}

int test_time() {
    assert(hour_12_to_24hour(12, false) == 0);
    assert(hour_12_to_24hour(1, false) == 1);
    assert(hour_12_to_24hour(8, false) == 8);
    assert(hour_12_to_24hour(11, false) == 11);
    assert(hour_12_to_24hour(12, true) == 12);
    assert(hour_12_to_24hour(1, true) == 13);
    assert(hour_12_to_24hour(8, true) == 20);
    assert(hour_12_to_24hour(11, true) == 23);

    assert(hour_24_to_12hour(0) == 12);
    assert(hour_24_to_12hour(1) == 1);
    assert(hour_24_to_12hour(8) == 8);
    assert(hour_24_to_12hour(11) == 11);
    assert(hour_24_to_12hour(12) == 12);
    assert(hour_24_to_12hour(13) == 1);
    assert(hour_24_to_12hour(20) == 8);
    assert(hour_24_to_12hour(23) == 11);
    assert(hour_24_to_12hour(23) == 11);

    assert(!hour_24_is_pm(0));
    assert(!hour_24_is_pm(1));
    assert(!hour_24_is_pm(8));
    assert(!hour_24_is_pm(11));
    assert(hour_24_is_pm(12));
    assert(hour_24_is_pm(18));
    assert(hour_24_is_pm(20));
    assert(hour_24_is_pm(23));

    for (enum weekday weekday = WEEKDAY_SUNDAY;
         weekday <= WEEKDAY_SATURDAY;
         weekday++)
    {
        assert(weekday_is_valid(weekday));
    }

    for (enum month month = MONTH_JANUARY; month <= MONTH_DECEMBER; month++) {
        assert(month_is_valid(month));
    }

    assert(!month_is_valid(MONTH_JANUARY - 1));
    assert(!month_is_valid(MONTH_DECEMBER + 1));

    assert(!weekday_is_valid((enum weekday)(WEEKDAY_SUNDAY - 1)));
    assert(!weekday_is_valid(WEEKDAY_SATURDAY + 1));

    assert(weekday_next(WEEKDAY_SUNDAY) == WEEKDAY_MONDAY);
    assert(weekday_next(WEEKDAY_MONDAY) == WEEKDAY_TUESDAY);
    assert(weekday_next(WEEKDAY_TUESDAY) == WEEKDAY_WEDNESDAY);
    assert(weekday_next(WEEKDAY_WEDNESDAY) ==
           WEEKDAY_THURSDAY);
    assert(weekday_next(WEEKDAY_THURSDAY) == WEEKDAY_FRIDAY);
    assert(weekday_next(WEEKDAY_FRIDAY) == WEEKDAY_SATURDAY);
    assert(weekday_next(WEEKDAY_SATURDAY) == WEEKDAY_SUNDAY);

    assert(weekday_prev(WEEKDAY_SUNDAY) == WEEKDAY_SATURDAY);
    assert(weekday_prev(WEEKDAY_MONDAY) == WEEKDAY_SUNDAY);
    assert(weekday_prev(WEEKDAY_TUESDAY) == WEEKDAY_MONDAY);
    assert(weekday_prev(WEEKDAY_WEDNESDAY) == WEEKDAY_TUESDAY);
    assert(weekday_prev(WEEKDAY_THURSDAY) == WEEKDAY_WEDNESDAY);
    assert(weekday_prev(WEEKDAY_FRIDAY) == WEEKDAY_THURSDAY);
    assert(weekday_prev(WEEKDAY_SATURDAY) == WEEKDAY_FRIDAY);

    test_match("%a");
    test_match("%A");
    test_match("%b");
    test_match("%B");
    //test_match("%c");
    test_match("%C");
    test_match("%d");
    test_match("%D");
    test_match("%e");
    test_match("%F");
    test_match("%g");
    test_match("%G");
    test_match("%h");
    test_match("%I");
    test_match("%j");
    test_match("%m");
    test_match("%n");
    test_match("%p");
    test_match("%r");
    test_match("%R");
    test_match("%S");
    test_match("%t");
    test_match("%T");
    test_match("%u");
    test_match("%U");
    test_match("%V");
    test_match("%w");
    test_match("%W");
    test_match("%x");
    test_match("%X");
    test_match("%y");
    test_match("%Y");
    test_match("%%");

    //test_match("%Ec");
    test_match("%EC");
    test_match("%Ex");
    test_match("%EX");
    test_match("%Ey");
    test_match("%EY");
    test_match("%Od");
    test_match("%Oe");
    test_match("%OH");
    test_match("%OI");
    test_match("%Om");
    test_match("%OM");
    test_match("%OS");
    test_match("%Ou");
    test_match("%OU");
    test_match("%OV");
    test_match("%Ow");
    test_match("%OW");
    test_match("%Oy");

    return 0;
}