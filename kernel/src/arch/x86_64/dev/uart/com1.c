/*
 * kernel/src/arch/x86_64/dev/uart/com1.c
 * © suhas pai
 */

#include "dev/uart/com1.h"
#include "dev/uart/8250.h"

void com1_init() {
    uart8250_init((port_t)0x3f8,
                  /*baudrate=*/115200,
                  /*in_freq=*/0,
                  /*reg_width=*/sizeof(uint8_t),
                  /*reg_shift=*/1);
}