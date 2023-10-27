/*
 * kernel/arch/x86_64/dev/cmos.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum cmos_register {
    CMOS_REGISTER_RTC_SECOND,
    CMOS_REGISTER_RTC_MINUTE = 0x02,
    CMOS_REGISTER_RTC_HOUR   = 0x04,
    CMOS_REGISTER_RTC_DAY    = 0x07,
    CMOS_REGISTER_RTC_MONTH,
    CMOS_REGISTER_RTC_YEAR,

    CMOS_REGISTER_RTC_STATUS_A = 0xA,
    CMOS_REGISTER_RTC_STATUS_B = 0xB,

    CMOS_REGISTER_FLOPPY_INFO = 0x10
};

uint8_t cmos_read(enum cmos_register reg);
void cmos_write(enum cmos_register reg, uint8_t data);