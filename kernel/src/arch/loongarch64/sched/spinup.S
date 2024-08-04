.global thread_spinup
thread_spinup:
    ld.d $ra, $a0, 8
    ld.d $tp, $a0, 16
    ld.d $a1, $a0, 32
    ld.d $a2, $a0, 40
    ld.d $a3, $a0, 48
    ld.d $a4, $a0, 56
    ld.d $a5, $a0, 64
    ld.d $a6, $a0, 72
    ld.d $a7, $a0, 80
    ld.d $t0, $a0, 88
    ld.d $t1, $a0, 96
    ld.d $t2, $a0, 104
    ld.d $t3, $a0, 112
    ld.d $t4, $a0, 120
    ld.d $t5, $a0, 128
    ld.d $t6, $a0, 136
    ld.d $t7, $a0, 144
    ld.d $t8, $a0, 152
    ld.d $fp, $a0, 160
    ld.d $s0, $a0, 168
    ld.d $s1, $a0, 176
    ld.d $s2, $a0, 184
    ld.d $s3, $a0, 192
    ld.d $s4, $a0, 200
    ld.d $s5, $a0, 208
    ld.d $s6, $a0, 216
    ld.d $s7, $a0, 224
    ld.d $s8, $a0, 232

    ld.d $sp, $a0, 0
    ld.d $a0, $a0, 24

    ertn
