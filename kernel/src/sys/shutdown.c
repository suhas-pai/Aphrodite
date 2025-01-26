/*
 * kernel/src/sys/shutdown.c
 * © suhas pai
 */

#include <lib/assert.h>
#include <uacpi/sleep.h>

#include "asm/irqs.h"

void system_shutdown() {
    /*
     * Prepare the system for shutdown.
     * This will run the \_PTS & \_SST methods, if they exist, as well as
     * some work to fetch the \_S5 and \_S0 values to make system wake
     * possible later on.
     */

    uacpi_status ret = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
    assert_msg(!uacpi_unlikely_error(ret),
               "sys/uacpi: failed to prepare for sleep: %s",
               uacpi_status_to_string(ret));

    /*
     * This is where we disable interrupts to prevent anything from
     * racing with our shutdown sequence below.
     */
    intr_disable();

    /*
     * Actually do the work of entering the sleep state by writing to the
     * hardware registers with the values we fetched during preparation.
     * This will also disable runtime events and enable only those that are
     * needed for wake.
     */
    ret = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
    assert_msg(!uacpi_unlikely_error(ret),
               "failed to enter sleep: %s",
               uacpi_status_to_string(ret));

    // Should be unreachable code
    verify_not_reached();
}
