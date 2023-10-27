/*
 * kernel/arch/x86_64/dev/time/rtc.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "dev/cmos.h"
#include "dev/printk.h"

#include "rtc.h"

enum cmos_rtc_reg_status_a_masks {
    __CMOS_RTC_REGSTATUS_A_UPDATE_IN_PROG = 1 << 7
};

enum cmos_rtc_reg_status_b_masks {
    __CMOS_RTC_REGSTATUS_B_DAYLIGHT_SAVING = 1 << 0,

    // Otherwise 12-hour mode
    __CMOS_RTC_REGSTATUS_B_24HR_MODE = 1 << 1,
    __CMOS_RTC_REGSTATUS_B_DATE_BINFMT  = 1 << 2,

    __CMOS_RTC_REGSTATUS_B_SQUAREWAVE = 1 << 0,
    __CMOS_RTC_REGSTATUS_B_INTR_ON_UPDATE_COMP = 1 << 3,
    __CMOS_RTC_REGSTATUS_B_INTR_ON_ALARM_COMP  = 1 << 4,

    __CMOS_RTC_REGSTATUS_B_PERIODIC_INT = 1 << 5,
    __CMOS_RTC_REGSTATUS_B_DISABLE_UPDATE = 1 << 6
};

#define MAX_ATTEMPTS 10

static inline bool rtc_wait_until_available() {
    for (uint64_t i = 0; i != MAX_ATTEMPTS; i++) {
        const uint8_t reg_a = cmos_read(CMOS_REGISTER_RTC_STATUS_A);
        if ((reg_a & __CMOS_RTC_REGSTATUS_A_UPDATE_IN_PROG) == 0) {
            return true;
        }
    }

    return false;
}

static inline bool read_reg_status_b(uint8_t *const result_out) {
    if (!rtc_wait_until_available()) {
        return false;
    }

    *result_out = cmos_read(CMOS_REGISTER_RTC_STATUS_B);
    return true;
}

static inline bool rtc_is_daylight_savings_enabled(const uint8_t reg_status_b) {
    return (reg_status_b & __CMOS_RTC_REGSTATUS_B_DAYLIGHT_SAVING);
}

static inline bool rtc_is_in_24_hour_mode(const uint8_t reg_status_b) {
    return (reg_status_b & __CMOS_RTC_REGSTATUS_B_24HR_MODE);
}

static inline bool rtc_is_date_in_binary_format(const uint8_t reg_status_b) {
    return (reg_status_b & __CMOS_RTC_REGSTATUS_B_DATE_BINFMT);
}

void rtc_init() {
    uint8_t reg_b = 0;
    if (!read_reg_status_b(&reg_b)) {
        printk(LOGLEVEL_WARN, "rtc: failed to read reg-status b\n");
        return;
    }

    reg_b |= __CMOS_RTC_REGSTATUS_B_DATE_BINFMT;

    cmos_write(CMOS_REGISTER_RTC_STATUS_B, reg_b);
    printk(LOGLEVEL_INFO, "rtc: init complete\n");
}

static inline uint8_t convert_bcd_to_binary(const uint8_t bcd) {
    return ((bcd & 0xF) + ((bcd >> 4) * 10));
}

static inline void read_rtc_cmos_info(struct rtc_cmos_info *const info_out) {
    rtc_wait_until_available();

    info_out->time.second = cmos_read(CMOS_REGISTER_RTC_SECOND);
    info_out->time.minute = cmos_read(CMOS_REGISTER_RTC_MINUTE);
    info_out->time.hour = cmos_read(CMOS_REGISTER_RTC_HOUR);
    info_out->time.day = cmos_read(CMOS_REGISTER_RTC_DAY);
    info_out->time.month = cmos_read(CMOS_REGISTER_RTC_MONTH);
    info_out->time.year = cmos_read(CMOS_REGISTER_RTC_YEAR);

    info_out->reg_status_a = cmos_read(CMOS_REGISTER_RTC_STATUS_A);
    info_out->reg_status_b = UINT16_MAX;

    uint8_t reg_b = 0;
    if (read_reg_status_b(&reg_b)) {
        info_out->reg_status_b = reg_b;
    }
}

bool rtc_read_cmos_info(struct rtc_cmos_info *const info_out) {
    /*
     * Read at least twice (and keep reading until both reads match) to avoid
     * reading in the middle of an rtc-update.
     */

    struct rtc_cmos_info info = RTC_CMOS_INFO_INIT();
    struct rtc_cmos_info check = RTC_CMOS_INFO_INIT();

    bool should_return = true;
    for (uint64_t i = 0; i != MAX_ATTEMPTS; i++) {
        read_rtc_cmos_info(&info);
        read_rtc_cmos_info(&check);

        if (info.reg_status_b != UINT16_MAX &&
            memcmp(&info, &check, sizeof(info)) == 0)
        {
            should_return = false;
            break;
        }
    }

    if (should_return) {
        printk(LOGLEVEL_WARN, "rtc: failed to read cmos info\n");
        return false;
    }

    // Get the century-number
    uint8_t century_lower_2_digits = cmos_read(get_acpi_info()->fadt->century);
    if (!rtc_is_date_in_binary_format(info.reg_status_b)) {
        const uint8_t hour = info.time.hour & 0x7F;

        info.time.second = convert_bcd_to_binary(info.time.second);
        info.time.minute = convert_bcd_to_binary(info.time.minute);
        info.time.hour = convert_bcd_to_binary(hour);
        info.time.day = convert_bcd_to_binary(info.time.day);
        info.time.month = convert_bcd_to_binary(info.time.month);
        info.time.year = convert_bcd_to_binary(info.time.year);

        century_lower_2_digits = convert_bcd_to_binary(century_lower_2_digits);
    }

    // We expect 24-hour real-time info
    if (!rtc_is_in_24_hour_mode(info.reg_status_b)) {
        const bool is_pm = info.time.hour & 0x80;
        info.time.hour = hour_12_to_24hour(info.time.hour, is_pm);
    }

    info.time.year += century_lower_2_digits * 100;
    info.time.weekday =
        day_of_month_to_weekday(info.time.year, info.time.month, info.time.day);

    *info_out = info;
    return true;
}

struct tm tm_from_rtc_cmos_info(struct rtc_cmos_info *const info) {
    return (struct tm){
        .tm_sec = info->time.second,
        .tm_min = info->time.minute,
        .tm_hour = info->time.hour,
        .tm_mday = info->time.day,
        .tm_wday =
            day_of_month_to_weekday(info->time.year,
                                    info->time.month,
                                    info->time.day),
        .tm_mon = month_to_tm_mon(info->time.month),
        .tm_year = year_to_tm_year(info->time.year),
        .tm_isdst = rtc_is_daylight_savings_enabled(info->reg_status_b)
    };
}