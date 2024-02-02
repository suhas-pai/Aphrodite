/*
 * kernel/src/arch/x86_64/asm/context.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "lib/macros.h"

struct register_context {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx;
    uint64_t rax;
};
