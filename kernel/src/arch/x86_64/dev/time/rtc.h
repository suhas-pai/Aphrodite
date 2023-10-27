/*
 * kernel/arch/x86_64/dev/time/rtc.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

struct rtc_time_info {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;

    enum month month : 8;
    enum weekday weekday : 8;

    uint64_t year;
};

#define RTC_TIME_INFO_INIT() \
    ((struct rtc_time_info) { \
        .second = 0, \
        .minute = 0, \
        .hour = 0,   \
        .day = 0,    \
        .month = MONTH_INVALID, \
        .weekday = WEEKDAY_INVALID, \
        .year = 0\
    })

struct rtc_cmos_info {
    struct rtc_time_info time;

    uint8_t reg_status_a;
    uint16_t reg_status_b;
};

#define RTC_CMOS_INFO_INIT() \
    ((struct rtc_cmos_info){ \
        .time = RTC_TIME_INFO_INIT(), \
        .reg_status_a = 0, \
        .reg_status_b = 0 \
    })

void rtc_init();
bool rtc_read_cmos_info(struct rtc_cmos_info *const info_out);

struct tm tm_from_rtc_cmos_info(struct rtc_cmos_info *const info);