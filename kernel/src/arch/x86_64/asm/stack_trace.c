/*
 * kernel/src/arch/x86_64/asm/stack_trace.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "stack_trace.h"

__optimize(3) struct stack_trace *stacktrace_top() {
    struct stack_trace *stk = NULL;
    asm volatile ("mov %%rbp, %0"
                  : "=r"(stk)
                  :: "memory");

    return stk;
}

__optimize(3)
struct stack_trace *stacktrace_next(struct stack_trace *const stk) {
    return stk->rbp;
}

__optimize(3) void print_stack_trace(const uint8_t max_lines) {
    struct stack_trace *stack = stacktrace_next(stacktrace_top());
    for (uint8_t i = 0;
         stack != NULL && i != max_lines;
         i++, stack = stacktrace_next(stack))
    {
        printk(LOGLEVEL_INFO, "\t%p\n", (void *)stack->rip);
    }
}

__optimize(3) void
print_stack_trace_from_top(struct stack_trace *const top,
                           const uint8_t max_lines)
{
    struct stack_trace *stack = stacktrace_next(top);
    for (uint8_t i = 0;
         stack != NULL && i != max_lines;
         i++, stack = stacktrace_next(stack))
    {
        printk(LOGLEVEL_INFO, "\t%p\n", (void *)stack->rip);
    }
}