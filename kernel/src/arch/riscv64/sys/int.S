/*
 * kernel/src/arch/riscv64/sys/int.S
 * © suhas pai
 */

.global thread_spinup;
.type thread_spinup,STT_FUNC;

thread_spinup:
    ld t0, 0(a0)
    csrw sepc, t0
    ld t0, 8(a0)
    csrw sstatus, t0

    ld sp, 16(a0)
    ld gp, 24(a0)
    ld tp, 32(a0)
    ld ra, 40(a0)
    ld t0, 48(a0)
    ld t1, 56(a0)
    ld t2, 64(a0)
    ld s0, 72(a0)
    ld s1, 80(a0)
    // don't load a0 yet
    ld a1, 96(a0)
    ld a2, 104(a0)
    ld a3, 112(a0)
    ld a4, 120(a0)
    ld a5, 128(a0)
    ld a6, 136(a0)
    ld a7, 144(a0)
    ld s2, 152(a0)
    ld s3, 160(a0)
    ld s4, 168(a0)
    ld s5, 176(a0)
    ld s6, 184(a0)
    ld s7, 192(a0)
    ld s8, 200(a0)
    ld s9, 208(a0)
    ld s10, 216(a0)
    ld s11, 224(a0)
    ld t3, 232(a0)
    ld t4, 240(a0)
    ld t5, 248(a0)
    ld t6, 256(a0)

    ld a0, 88(a0)
    sret

.align 6
.global isr_interrupt_entry;
.type isr_interrupt_entry,STT_FUNC;

isr_interrupt_entry:
    sd sp, -248(sp)
    addi sp, sp, -264

    sd gp, 24(sp)
    sd tp, 32(sp)
    sd ra, 40(sp)
    sd t0, 48(sp)
    sd t1, 56(sp)
    sd t2, 64(sp)
    sd s0, 72(sp)
    sd s1, 80(sp)
    sd a0, 88(sp)
    sd a1, 96(sp)
    sd a2, 104(sp)
    sd a3, 112(sp)
    sd a4, 120(sp)
    sd a5, 128(sp)
    sd a6, 136(sp)
    sd a7, 144(sp)
    sd s2, 152(sp)
    sd s3, 160(sp)
    sd s4, 168(sp)
    sd s5, 176(sp)
    sd s6, 184(sp)
    sd s7, 192(sp)
    sd s8, 200(sp)
    sd s9, 208(sp)
    sd s10, 216(sp)
    sd s11, 224(sp)
    sd t3, 232(sp)
    sd t4, 240(sp)
    sd t5, 248(sp)
    sd t6, 256(sp)

    csrr t0, sepc
    sd t0, 0(sp)

    csrr t0, sstatus
    sd t0, 8(sp)

    csrr a0, scause
    mv a1, sp

    call isr_handle_interrupt

    ld t0, 0(sp)
    csrw sepc, t0
    ld t0, 8(sp)
    csrw sstatus, t0

    ld gp, 24(sp)
    ld tp, 32(sp)
    ld ra, 40(sp)
    ld t0, 48(sp)
    ld t1, 56(sp)
    ld t2, 64(sp)
    ld s0, 72(sp)
    ld s1, 80(sp)
    ld a0, 88(sp)
    ld a1, 96(sp)
    ld a2, 104(sp)
    ld a3, 112(sp)
    ld a4, 120(sp)
    ld a5, 128(sp)
    ld a6, 136(sp)
    ld a7, 144(sp)
    ld s2, 152(sp)
    ld s3, 160(sp)
    ld s4, 168(sp)
    ld s5, 176(sp)
    ld s6, 184(sp)
    ld s7, 192(sp)
    ld s8, 200(sp)
    ld s9, 208(sp)
    ld s10, 216(sp)
    ld s11, 224(sp)
    ld t3, 232(sp)
    ld t4, 240(sp)
    ld t5, 248(sp)
    ld t6, 256(sp)

    sc.d zero, zero, 0(sp)
    ld sp, 16(sp)

    sret