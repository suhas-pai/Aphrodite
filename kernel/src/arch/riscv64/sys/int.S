/*
 * kernel/src/arch/riscv64/sys/int.S
 * © suhas pai
 */

.global context_switch;
.type context_switch,STT_FUNC;

context_switch:
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd tp, 16(a0)
    sd s0, 24(a0)
    sd s1, 32(a0)
    sd s2, 40(a0)
    sd s3, 48(a0)
    sd s4, 56(a0)
    sd s5, 64(a0)
    sd s6, 72(a0)
    sd s7, 80(a0)
    sd s8, 88(a0)
    sd s9, 96(a0)
    sd s10, 104(a0)
    sd s11, 112(a0)

    ld s11, 112(a1)
    ld s10, 104(a1)
    ld s9, 96(a1)
    ld s8, 88(a1)
    ld s7, 80(a1)
    ld s6, 72(a1)
    ld s5, 64(a1)
    ld s4, 56(a1)
    ld s3, 48(a1)
    ld s2, 40(a1)
    ld s1, 32(a1)
    ld s0, 24(a1)
    ld tp, 16(a1)
    ld sp, 8(a1)
    ld ra, 0(a1)

    ret

.align 6
.global isr_interrupt_entry;
.type isr_interrupt_entry,STT_FUNC;

isr_interrupt_entry:
    addi sp, sp, -160 // 32-byte aligned stack

    sd t6, 136(sp)
    sd t5, 128(sp)
    sd t4, 120(sp)
    sd t3, 112(sp)
    sd t2, 104(sp)
    sd t1, 96(sp)
    sd t0, 88(sp)
    sd a7, 80(sp)
    sd a6, 72(sp)
    sd a5, 64(sp)
    sd a4, 56(sp)
    sd a3, 48(sp)
    sd a2, 40(sp)
    sd a1, 32(sp)
    sd a0, 24(sp)
    sd ra, 16(sp)

    csrr t0, sstatus
    sd t0, 8(sp)

    csrr a0, scause
    csrr a1, sepc

    sd a1, 0(sp)
    mv a2, sp

    jal isr_handle_interrupt

    ld t0, 0(sp)
    csrw sepc, t0
    ld t0, 8(sp)
    csrw sstatus, t0

    ld ra, 16(sp)
    ld a0, 24(sp)
    ld a1, 32(sp)
    ld a2, 40(sp)
    ld a3, 48(sp)
    ld a4, 56(sp)
    ld a5, 64(sp)
    ld a6, 72(sp)
    ld a7, 80(sp)
    ld t0, 88(sp)
    ld t1, 96(sp)
    ld t2, 104(sp)
    ld t3, 112(sp)
    ld t4, 120(sp)
    ld t5, 128(sp)
    ld t6, 136(sp)

    addi sp, sp, 160
    sret