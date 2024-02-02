#define context_size 280
#define spsr_index (31 * 8)
#define elr_index (32 * 8)
#define sp_index (33 * 8)
#define esr_index (34 * 8)

.cfi_sections .eh_frame, .debug_frame

.macro save_context
    sub sp, sp, context_size

    stp x0, x1, [sp, #(0 * 16)]
    .cfi_offset x0, 0
    .cfi_offset x1, 8

    stp x2, x3, [sp, #(1 * 16)]
    .cfi_offset x2, 16
    .cfi_offset x3, 24

    stp x4, x5, [sp, #(2 * 16)]
    .cfi_offset x4, 32
    .cfi_offset x5, 40

    stp x6, x7, [sp, #(3 * 16)]
    .cfi_offset x6, 48
    .cfi_offset x7, 56

    stp x8, x9, [sp, #(4 * 16)]
    .cfi_offset x8, 64
    .cfi_offset x9, 72

    stp x10, x11, [sp, #(5 * 16)]
    .cfi_offset x10, 80
    .cfi_offset x11, 88

    stp x12, x13, [sp, #(6 * 16)]
    .cfi_offset x12, 96
    .cfi_offset x13, 104

    stp x14, x15, [sp, #(7 * 16)]
    .cfi_offset x14, 112
    .cfi_offset x15, 120

    stp x16, x17, [sp, #(8 * 16)]
    .cfi_offset x16, 128
    .cfi_offset x17, 136

    stp x18, x19, [sp, #(9 * 16)]
    .cfi_offset x18, 144
    .cfi_offset x19, 152

    stp x20, x21, [sp, #(10 * 16)]
    .cfi_offset x20, 160
    .cfi_offset x21, 168

    stp x22, x23, [sp, #(11 * 16)]
    .cfi_offset x22, 176
    .cfi_offset x23, 184

    stp x24, x25, [sp, #(12 * 16)]
    .cfi_offset x24, 192
    .cfi_offset x25, 200

    stp x26, x27, [sp, #(13 * 16)]
    .cfi_offset x26, 208
    .cfi_offset x27, 216

    stp x28, x29, [sp, #(14 * 16)]
    .cfi_offset x28, 224
    .cfi_offset x29, 232

    str x30, [sp, #(15 * 16)]
    .cfi_offset x30, 240

    mrs x0, spsr_el1
    str x0, [sp, #spsr_index]
    .cfi_offset pstate, #spsr_index

    mrs x0, elr_el1
    str x0, [sp, #elr_index]
    .cfi_offset ip, #elr_index

    mrs x0, sp_el0
    str x0, [sp, #sp_index]

    mrs x0, esr_el1
    str x0, [sp, #esr_index]

    mov x0, sp
.endm

.macro restore_context
    ldr x0, [sp, #spsr_index]
    msr spsr_el1, x0
    ldr x0, [sp, #elr_index]
    msr elr_el1, x0
    ldr x0, [sp, #sp_index]
    msr sp_el0, x0

    ldp x0, x1, [sp, #(0 * 16)]
    ldp x2, x3, [sp, #(1 * 16)]
    ldp x4, x5, [sp, #(2 * 16)]
    ldp x6, x7, [sp, #(3 * 16)]
    ldp x8, x9, [sp, #(4 * 16)]
    ldp x10, x11, [sp, #(5 * 16)]
    ldp x12, x13, [sp, #(6 * 16)]
    ldp x14, x15, [sp, #(7 * 16)]
    ldp x16, x17, [sp, #(8 * 16)]
    ldp x18, x19, [sp, #(9 * 16)]
    ldp x20, x21, [sp, #(10 * 16)]
    ldp x22, x23, [sp, #(11 * 16)]
    ldp x24, x25, [sp, #(12 * 16)]
    ldp x26, x27, [sp, #(13 * 16)]
    ldp x28, x29, [sp, #(14 * 16)]
    ldr x30, [sp, #(15 * 16)]

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
    .cfi_startproc simple
    .cfi_signal_frame

    save_context
    b handle_interrupt
    restore_context

    eret
    .cfi_endproc

.extern handle_exception
ivt_exception_func:
    .cfi_startproc simple
    .cfi_signal_frame

    save_context
    b handle_exception
    restore_context

    eret
    .cfi_endproc

.extern handle_sexception
ivt_sexception_func:
    .cfi_startproc simple
    .cfi_signal_frame

    save_context
    b handle_sexception
    restore_context

    eret
    .cfi_endproc

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
    ivt_entry ivt_sexception_func // Error EL1h

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
