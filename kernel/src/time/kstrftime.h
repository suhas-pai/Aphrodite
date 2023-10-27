/*
 * kernel/dev/time/kstrftime.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string.h"
#include "lib/time.h"

struct string kstrftime(const char *format, const struct tm *tm);
void printk_strftime(const char *format, const struct tm *tm);