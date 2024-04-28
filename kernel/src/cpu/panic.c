/*
 * kernel/src/cpu/panic.c
 * © suhas pai
 */

#include "asm/irqs.h"
#if __has_include("asm/stack_trace.h")
    #include "asm/stack_trace.h"
#endif /* __has_include("asm/stack_trace.h") */

#include "cpu/util.h"
#include "dev/printk.h"

__optimize(3) void panic(const char *const fmt, ...) {
    disable_interrupts();

    va_list list;
    va_start(list, fmt);

    vprintk(LOGLEVEL_CRITICAL, fmt, list);
    va_end(list);

#if __has_include("asm/stack_trace.h")
    print_stack_trace(/*max_lines=*/10);
#endif /* __has_include("asm/stack_trace.h") */

    cpu_idle();
}