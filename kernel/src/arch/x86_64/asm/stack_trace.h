/*
 * kernel/arch/x86_64/asm/stack_trace.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct stack_trace {
    struct stack_trace *rbp;
    uint64_t rip;
};

struct stack_trace *stacktrace_top();
struct stack_trace *stacktrace_next(struct stack_trace *stk);

void print_stack_trace(uint8_t max_lines);