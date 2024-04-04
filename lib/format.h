/*
 * lib/format.h
 * © suhas pai
 */

#pragma once
#include "adt/string.h"

__printf_format(3, 4) uint32_t
format_to_buffer(char *buffer_in, uint32_t buffer_len, const char *format, ...);

uint32_t
vformat_to_buffer(char *buffer,
                  uint32_t buffer_len,
                  const char *format,
                  va_list list);

__printf_format(2, 3)
uint32_t format_to_string(struct string *string, const char *format, ...);

uint32_t
vformat_to_string(struct string *string, const char *format, va_list list);
