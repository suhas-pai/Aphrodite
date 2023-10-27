/*
 * lib/format.h
 * © suhas pai
 */

#pragma once

#include <stdarg.h>
#include <stdint.h>

#include "adt/string.h"

__printf_format(3, 4)
uint64_t
format_to_buffer(char *buffer_in, uint64_t buffer_len, const char *format, ...);

uint64_t
vformat_to_buffer(char *buffer,
                  uint64_t buffer_len,
                  const char *format,
                  va_list list);

__printf_format(2, 3)
uint64_t format_to_string(struct string *string, const char *format, ...);

uint64_t
vformat_to_string(struct string *string, const char *format, va_list list);