/*
 * kernel/dev/printk.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "cpu/spinlock.h"
#include "lib/parse_printf.h"

#include "printk.h"

static struct terminal *_Atomic g_first_term = NULL;

__optimize(3) void printk_add_terminal(struct terminal *const term) {
    atomic_store(&term->next, g_first_term);
    atomic_store(&g_first_term, term);
}

__optimize(3)
void printk(const enum log_level loglevel, const char *const string, ...) {
    va_list list;
    (void)loglevel;

    va_start(list, string);
    vprintk(loglevel, string, list);
    va_end(list);
}

// FIXME: Allocate a formatted-string over this approach
__optimize(3) static uint64_t
write_char(struct printf_spec_info *const spec_info,
           void *const cb_info,
           const char ch,
           const uint64_t amount,
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

__optimize(3) static uint64_t
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

void putk(const char *const string) {
    putk_sv(sv_create_length(string, strlen(string)));
}

void putk_sv(const struct string_view sv) {
    for (struct terminal *term = atomic_load(&g_first_term);
         term != NULL;
         term = atomic_load(&term->next))
    {
        term->emit_sv(term, sv);
    }
}

__optimize(3) void
vprintk(const enum log_level loglevel, const char *const string, va_list list) {
    (void)loglevel;

    static struct spinlock lock = SPINLOCK_INIT();
    const int flag = spin_acquire_with_irq(&lock);

    parse_printf(string,
                 write_char,
                 /*char_cb_info=*/NULL,
                 write_sv,
                 /*sv_cb_info=*/NULL,
                 list);

    spin_release_with_irq(&lock, flag);
}