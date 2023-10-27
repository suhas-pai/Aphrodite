/*
 * lib/strftime.h
 * © suhas pai
 */

#pragma once
#include "time.h"

uint64_t
time_format_to_string_buffer(char *buffer_in,
                             uint64_t buffer_len,
                             const char *format,
                             const struct tm *tm);

uint64_t get_length_of_time_format(const char *format, const struct tm *tm);