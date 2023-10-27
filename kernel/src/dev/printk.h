/*
 * kernel/dev/printk.h
 * © suhas pai
 */

#pragma once

#include <stdarg.h>

#include "lib/adt/string_view.h"
#include "lib/inttypes.h"

struct terminal;
struct terminal {
    struct terminal *_Atomic next;

    void (*emit_ch)(struct terminal *term, char ch, uint32_t amt);
    void (*emit_sv)(struct terminal *term, struct string_view sv);

    // Useful for panic()
    void (*bust_locks)(struct terminal *);
};

void printk_add_terminal(struct terminal *term);

enum log_level {
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR,
    LOGLEVEL_CRITICAL
};

__printf_format(2, 3)
void printk(enum log_level loglevel, const char *string, ...);
void vprintk(enum log_level loglevel, const char *string, va_list list);

void putk(const char *string);
void putk_sv(struct string_view sv);