/*
 * kernel/src/time/kstrftime.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/string.h"

#include "dev/printk.h"
#include "lib/time.h"

struct string kstrftime(const char *format, const struct tm *tm);

void
printk_strftime(const enum log_level level,
                const char *format,
                const struct tm *tm);