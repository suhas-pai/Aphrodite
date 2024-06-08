/*
 * kernel/src/arch/x86_64/dev/init.c
 * © suhas pai
 */

#include "dev/ide/init.h"
#include "dev/ps2/driver.h"

#include "dev/time/hpet.h"
#include "dev/time/rtc.h"

#include "acpi/api.h"
#include "asm/rdrand.h"

#include "dev/printk.h"
#include "time/kstrftime.h"

extern bool g_found_ide;

void arch_init_dev() {
    const struct acpi_fadt *const fadt = get_acpi_info()->fadt;
    if (fadt != NULL && fadt->iapc_boot_arch_flags & __ACPI_FADT_IAPC_BOOT_8042)
    {
        ps2_init();
    } else {
        printk(LOGLEVEL_WARN, "dev: ps2 keyboard/mouse are not supported\n");
    }

    if (rtc_init()) {
        struct rtc_cmos_info rtc_info = RTC_CMOS_INFO_INIT();
        if (rtc_read_cmos_info(&rtc_info)) {
            const struct tm tm = tm_from_rtc_cmos_info(&rtc_info);
            struct string string = kstrftime("%c", &tm);

            printk(LOGLEVEL_INFO,
                "dev: rtc time is " STRING_FMT "\n",
                STRING_FMT_ARGS(string));

            string_destroy(&string);
        }
    }

    const struct acpi_hpet *const hpet =
        (const struct acpi_hpet *)acpi_lookup_sdt("HPET");

    assert_msg(hpet != NULL, "dev: hpet not found. aborting init");
    hpet_init(hpet);

    uint64_t rand = 0;
    if (rdrand(&rand)) {
        printk(LOGLEVEL_INFO, "dev: testing random, got: %" PRIu64 "\n", rand);
    } else {
        printk(LOGLEVEL_WARN, "dev: failed to execute rdrand instruction\n");
    }
}

void arch_init_dev_drivers() {
    if (!g_found_ide) {
        ide_init(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
    } else {
        printk(LOGLEVEL_INFO, "found ide\n");
    }
}