/*
 * tests/common.h
 * © suhas pai
 */

#pragma once

#include <string.h>
#include <stdio.h>

#include <lib/adt/string_view.h>

static void check_strings(const char *expected, const char *const got) {
    if (strcmp(expected, got) != 0) {
        printf("check_strings(): FAIL. expected \"%s\" got \"%s\"\n",
               expected,
               got);
    }
}

static void
check_sv(const struct string_view actual, const struct string_view expected) {
    if (!sv_equals(expected, actual)) {
        printf("check_sv(): FAIL. expected \"" SV_FMT "\" "
               "actual \"" SV_FMT "\"\n",
               SV_FMT_ARGS(expected),
               SV_FMT_ARGS(actual));
    }
}
