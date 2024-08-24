/*
 * kernel/src/dev/printk.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "asm/irqs.h"

#include "cpu/cpu_info.h"
#include "cpu/spinlock.h"

#include "lib/parse_printf.h"
#include "sched/thread.h"
#include "printk.h"

static struct terminal *_Atomic g_first_term = NULL;

__debug_optimize(3) void printk_add_terminal(struct terminal *const term) {
    atomic_store(&term->next, g_first_term);
    atomic_store(&g_first_term, term);
}

__debug_optimize(3)
void printk(const enum log_level loglevel, const char *const string, ...) {
    (void)loglevel;
    va_list list;

    va_start(list, string);
    vprintk(loglevel, string, list);
    va_end(list);
}

// FIXME: Allocate a formatted-string over this approach
__debug_optimize(3) static uint32_t
write_char(struct printf_spec_info *const spec_info,
           void *const cb_info,
           const char ch,
           const uint32_t amount,
           bool *const cont_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)cont_out;

    for (struct terminal *term = atomic_load(&g_first_term);
         term != NULL;
         term = atomic_load(&term->next))
    {
        term->emit_ch(term, ch, amount);
    }

    return amount;
}

__debug_optimize(3) static uint32_t
write_sv(struct printf_spec_info *const spec_info,
         void *const cb_info,
         const struct string_view sv,
         bool *const cont_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)cont_out;

    for (struct terminal *term = atomic_load(&g_first_term);
         term != NULL;
         term = atomic_load(&term->next))
    {
        term->emit_sv(term, sv);
    }

    return sv.length;
}

static struct spinlock g_print_lock = SPINLOCK_INIT();

#define ACQUIRE_OR_BUST_LOCK() \
    const bool flag = disable_irqs_if_enabled(); \
    const bool in_exception = cpu_in_bad_state(); \
\
    if (in_exception) { \
        g_print_lock = SPINLOCK_INIT(); \
    } else { \
        spin_acquire(&g_print_lock); \
    }

#define RELEASE_LOCK() \
    if (!in_exception) { \
        spin_release(&g_print_lock); \
    } \
\
    enable_irqs_if_flag(flag);

__debug_optimize(3)
void putk(const enum log_level level, const char *const string) {
    ACQUIRE_OR_BUST_LOCK();
    putk_sv(level, sv_create_length(string, strlen(string)));
    RELEASE_LOCK();
}

__debug_optimize(3)
void putk_sv(const enum log_level level, const struct string_view sv) {
    (void)level;

    ACQUIRE_OR_BUST_LOCK();
    write_sv(/*spec_info=*/NULL, /*cb_info=*/NULL, sv, /*cont_out=*/NULL);
    RELEASE_LOCK();
}

__debug_optimize(3)
void vprintk_internal(const char *const string, va_list list) {
    parse_printf(string,
                 write_char,
                 /*char_cb_info=*/NULL,
                 write_sv,
                 /*sv_cb_info=*/NULL,
                 list);
}

__debug_optimize(3) static void printk_internal(const char *const string, ...) {
    va_list list;

    va_start(list, string);
    vprintk_internal(string, list);
    va_end(list);
}

__debug_optimize(3) void
vprintk(const enum log_level loglevel, const char *const string, va_list list) {
    (void)loglevel;
    ACQUIRE_OR_BUST_LOCK();

    printk_internal("[cpu %" PRIu32 "] ", cpu_get_id(current_thread()->cpu));
    parse_printf(string,
                 write_char,
                 /*char_cb_info=*/NULL,
                 write_sv,
                 /*sv_cb_info=*/NULL,
                 list);

    RELEASE_LOCK();
}