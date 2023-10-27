#define context_size 272
#define spsr_index (16 * 16)
#define elr_index (17 * 16)
#define sp_index (18 * 16)

.macro save_context
    sub sp, sp, context_size

    stp x0, x1,     [sp, #(0 * 8)]
    stp x2, x3,     [sp, #(1 * 16)]
    stp x4, x5,     [sp, #(2 * 16)]
    stp x6, x7,     [sp, #(3 * 16)]
    stp x8, x9,     [sp, #(4 * 16)]
    stp x10, x11,   [sp, #(5 * 16)]
    stp x12, x13,   [sp, #(6 * 16)]
    stp x14, x15,   [sp, #(7 * 16)]
    stp x16, x17,   [sp, #(8 * 16)]
    stp x18, x19,   [sp, #(9 * 16)]
    stp x20, x21,   [sp, #(10 * 16)]
    stp x22, x23,   [sp, #(11 * 16)]
    stp x24, x25,   [sp, #(12 * 16)]
    stp x26, x27,   [sp, #(13 * 16)]
    stp x28, x29,   [sp, #(14 * 16)]
    str x30,        [sp, #(15 * 16)]

    mrs x0, spsr_el1
    str x0, [sp, #spsr_index]
    mrs x0, elr_el1
    str x0, [sp, #elr_index]
    mrs x0, sp_el0
    str x0, [sp, #sp_index]

    mov x0, sp
.endm

.macro restore_context
    ldr x0, [sp, #spsr_index]
    msr spsr_el1, x0
    ldr x0, [sp, #elr_index]
    msr elr_el1, x0
    ldr x0, [sp, #sp_index]
    msr sp_el0, x0

    ldp x0, x1,     [sp, #(0 * 16)]
    ldp x2, x3,     [sp, #(1 * 16)]
    ldp x4, x5,     [sp, #(2 * 16)]
    ldp x6, x7,     [sp, #(3 * 16)]
    ldp x8, x9,     [sp, #(4 * 16)]
    ldp x10, x11,   [sp, #(5 * 16)]
    ldp x12, x13,   [sp, #(6 * 16)]
    ldp x14, x15,   [sp, #(7 * 16)]
    ldp x16, x17,   [sp, #(8 * 16)]
    ldp x18, x19,   [sp, #(9 * 16)]
    ldp x20, x21,   [sp, #(10 * 16)]
    ldp x22, x23,   [sp, #(11 * 16)]
    ldp x24, x25,   [sp, #(12 * 16)]
    ldp x26, x27,   [sp, #(13 * 16)]
    ldp x28, x29,   [sp, #(14 * 16)]
    ldr x30,        [sp, #(15 * 16)]

    add sp, sp, context_size
.endm

.macro ivt_entry label
    .align 7
    b \label
.endm

.macro ivt_unimpl_entry
    .align 7
    wfe
.endm

.extern handle_interrupt
ivt_interrupt_func:
    save_context
    b handle_interrupt
    restore_context
    eret

.extern handle_exception
ivt_exception_func:
    save_context
    b handle_exception
    restore_context
    eret

// VBAR_EL1 takes an address aligned to 2^11=2kib
.global ivt_el1
.align 11
ivt_el1:
    // Exceptions taken from Current EL, with SP_EL0
    ivt_entry ivt_exception_func // Synchronous EL1t
    ivt_entry ivt_interrupt_func // IRQ EL1t
    ivt_entry ivt_exception_func // FIQ EL1t
    ivt_entry ivt_exception_func // Error EL1t

    // Exceptions taken from Current EL, with SP_ELx, x>0
    ivt_entry ivt_exception_func // Synchronous EL1h
    ivt_entry ivt_interrupt_func // IRQ EL1h
    ivt_entry ivt_exception_func // FIQ EL1h
    ivt_entry ivt_exception_func // Error EL1h

    // Exceptions from Lower EL, where causing application is in AArch64 mode
    ivt_entry ivt_exception_func // Synchronous 64-bit EL1h
    ivt_entry ivt_interrupt_func // IRQ 64-bit EL1h
    ivt_entry ivt_exception_func // FIQ 64-bit EL1h
    ivt_entry ivt_exception_func // Error 64-bit EL1h

    // aarch32 entries
    ivt_unimpl_entry
    ivt_unimpl_entry
    ivt_unimpl_entry
    ivt_unimpl_entry
