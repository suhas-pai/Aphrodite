#define context_size 280
#define spsr_index (31 * 8)
#define elr_index (32 * 8)
#define sp_index (33 * 8)
#define esr_index (34 * 8)

.cfi_sections .eh_frame, .debug_frame

.macro save_context
    mov x0, sp
    sub x0, x0, context_size

    str x1, [x0, #8]
    stp x2, x3, [x0, #(1 * 16)]
    stp x4, x5, [x0, #(2 * 16)]
    stp x6, x7, [x0, #(3 * 16)]
    stp x8, x9, [x0, #(4 * 16)]
    stp x10, x11, [x0, #(5 * 16)]
    stp x12, x13, [x0, #(6 * 16)]
    stp x14, x15, [x0, #(7 * 16)]
    stp x16, x17, [x0, #(8 * 16)]
    stp x18, x19, [x0, #(9 * 16)]
    stp x20, x21, [x0, #(10 * 16)]
    stp x22, x23, [x0, #(11 * 16)]
    stp x24, x25, [x0, #(12 * 16)]
    stp x26, x27, [x0, #(13 * 16)]
    stp x28, x29, [x0, #(14 * 16)]
    str x30, [x0, #(15 * 16)]

    mov x1, sp
    mov sp, x0

    mov x0, xzr
    msr tpidrro_el0, x0

    mrs x0, spsr_el1
    str x0, [sp, #spsr_index]

    mrs x0, elr_el1
    str x0, [sp, #elr_index]

    mrs x0, sp_el0
    str x0, [sp, #sp_index]

    mrs x0, esr_el1
    str x0, [sp, #esr_index]

    mrs x0, tpidrro_el0
    str x0, [sp, #0]

    mov x29, xzr
    mov x0, sp
.endm

.macro restore_context
    ldr x0, [sp, #spsr_index]
    msr spsr_el1, x0
    ldr x0, [sp, #elr_index]
    msr elr_el1, x0
    ldr x0, [sp, #sp_index]
    msr sp_el0, x0

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

    mov x0, sp
    ldp x0, x1, [x0, #(0 * 16)]
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
    bl handle_interrupt
    restore_context

    eret

.extern handle_sync_exception
ivt_sync_exception_func:
    save_context
    bl handle_sync_exception
    restore_context

    eret

.extern handle_async_exception
ivt_async_exception_func:
    save_context
    bl handle_async_exception
    restore_context

    eret

.extern handle_invalid_exception
ivt_invalid_exception_func:
    save_context
    bl handle_invalid_exception
    restore_context

    eret

// VBAR_EL1 takes an address aligned to 2^11=2kib
.global ivt_el1
.align 11
ivt_el1:
    // Exceptions taken from Current EL, with SP_EL0
    ivt_entry ivt_sync_exception_func // Synchronous EL1t
    ivt_entry ivt_interrupt_func // IRQ EL1t
    ivt_entry ivt_invalid_exception_func // FIQ EL1t
    ivt_entry ivt_async_exception_func // Error EL1t

    // Exceptions taken from Current EL, with SP_ELx, x>0
    ivt_entry ivt_sync_exception_func // Synchronous EL1h
    ivt_entry ivt_interrupt_func // IRQ EL1h
    ivt_entry ivt_invalid_exception_func // FIQ EL1h
    ivt_entry ivt_async_exception_func // Error EL1h

    // Exceptions from Lower EL, where causing application is in AArch64 mode
    ivt_entry ivt_sync_exception_func // Synchronous 64-bit EL1h
    ivt_entry ivt_interrupt_func // IRQ 64-bit EL1h
    ivt_entry ivt_sync_exception_func // FIQ 64-bit EL1h
    ivt_entry ivt_async_exception_func // Error 64-bit EL1h

    // aarch32 entries
    ivt_unimpl_entry
    ivt_unimpl_entry
    ivt_unimpl_entry
    ivt_unimpl_entry
