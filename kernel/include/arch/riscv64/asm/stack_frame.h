/*
 * kernel/src/arch/riscv64/asm/stack_frame.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct stack_frame {
    uint64_t ra;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t fp;
    uint64_t s1;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
};

#define STACK_FRAME_INIT(arg) \
    ((struct stack_frame){ \
        .ra = 0, \
        .t0 = 0, \
        .t1 = 0, \
        .t2 = 0, \
        .fp = 0, \
        .s1 = 0, \
        .a0 = (uint64_t)(arg), \
        .a1 = 0, \
        .a2 = 0, \
        .a3 = 0, \
        .a4 = 0, \
        .a5 = 0, \
        .a6 = 0, \
        .a7 = 0, \
        .s2 = 0, \
        .s3 = 0, \
        .s4 = 0, \
        .s5 = 0, \
        .s6 = 0, \
        .s7 = 0, \
        .s8 = 0, \
        .s9 = 0, \
        .s10 = 0, \
        .s11 = 0, \
        .t3 = 0, \
        .t4 = 0, \
        .t5 = 0, \
        .t6 = 0, \
    })
