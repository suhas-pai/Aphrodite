/*
 * tests/common.h
 * © suhas pai
 */

#pragma once

#include <string.h>
#include <stdio.h>

static void check_strings(const char *expected, const char *const got) {
    if (strcmp(expected, got) != 0) {
        printf("check_strings(): FAIL. expected \"%s\" got \"%s\"\n",
               expected,
               got);
    }
}