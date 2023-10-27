/*
 * kernel/arch/x86_64/dev/ps2.c
 * © suhas pai
 */

#include "dev/ps2/keyboard.h"
#include "asm/pause.h"

#include "dev/port.h"
#include "dev/printk.h"

#include "driver.h"

#define RETRY_LIMIT 10

int16_t ps2_read_input_byte() {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        const uint8_t byte = port_in8(PORT_PS2_READ_STATUS);
        if ((byte & __PS2_STATUS_REG_OUTPUT_BUFFER_FULL) == 0) {
            cpu_pause();
            continue;
        }

        return port_in8(PORT_PS2_INPUT_BUFFER);
    }

    return -1;
}

bool ps2_write(const uint16_t port, const uint8_t value) {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        const uint8_t byte = port_in8(PORT_PS2_READ_STATUS);
        if (byte & __PS2_STATUS_REG_INPUT_BUFFER_FULL) {
            cpu_pause();
            continue;
        }

        port_out8(port, value);
        return true;
    }

    return false;
}

bool ps2_send_command(const enum ps2_command command) {
    return ps2_write(PORT_PS2_WRITE_CMD, command);
}

int16_t ps2_read_config() {
    if (!ps2_send_command(PS2_CMD_READ_INPUT_BUFFER_BYTE)) {
        return -1;
    }

    return ps2_read_input_byte();
}

bool ps2_write_config(const uint8_t value) {
    if (!ps2_send_command(PS2_CMD_WRITE_INPUT_BUFFER_BYTE)) {
        return false;
    }

    if (!ps2_write(PORT_PS2_INPUT_BUFFER, value)) {
        return false;
    }

    return true;
}

bool send_byte_to_port(const enum ps2_port_id device, const uint8_t byte) {
    if (device != PS2_FIRST_PORT) {
        if (!ps2_send_command(PS2_CMD_WRITE_NEXT_BYTE_TO_2ND_DEVICE_INPUT)) {
            return false;
        }
    }

    return ps2_write(PORT_PS2_INPUT_BUFFER, byte);
}

int16_t ps2_send_to_port(const enum ps2_port_id device, const uint8_t byte) {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        if (!send_byte_to_port(device, byte)) {
            return -1;
        }

        const int16_t response = ps2_read_input_byte();
        if (response != PS2_RESPONSE_RESEND) {
            return response;
        }
    }

    return PS2_RESPONSE_FAIL;
}

static bool
ps2_get_device_kind(const enum ps2_port_id device_id,
                    enum ps2_device_kind *const result_out)
{
    /*
     * Stage 1:
     *  Send the "disable scanning" command 0xF5 to the device
     *  Wait for the device to send "ACK" back (0xFA)
     */

    ps2_send_to_port(device_id, PS2_CMD_DISABLE_SCANNING);
    ps2_read_input_byte();

    /*
     * Stage 2:
     *  Send the "identify" command 0xF2 to the device
     *  Wait for the device to send "ACK" back (0xFA).
     *
     * NOTE: A device may also return 0xAA.
     */

    const int16_t identity_cmd_response =
        ps2_send_to_port(device_id, PS2_CMD_IDENTIFY);

    if (identity_cmd_response != PS2_RESPONSE_ACKNOWLEDGE) {
        printk(LOGLEVEL_INFO,
               "ps2: identify command for device %" PRIu8 " failed with "
               "response: 0x%" PRIx16 "\n",
               device_id,
               identity_cmd_response);
        return false;
    }

    /* NOTE: This is a hack. We're not supposed to have to have these checks. */

    const int16_t first = ps2_read_input_byte();
    const int16_t second = ps2_read_input_byte();

    printk(LOGLEVEL_INFO,
           "ps2: port %" PRIu8 " identity: 0x%" PRIx16 " 0x%" PRIx16 "\n",
           device_id,
           first,
           second);

    const int16_t enable_scanning_response =
        ps2_send_to_port(device_id, PS2_CMD_ENABLE_SCANNING);

    if (enable_scanning_response != PS2_RESPONSE_ACKNOWLEDGE) {
        printk(LOGLEVEL_WARN,
               "ps2: enable scanning for device %" PRIu8 " failed with "
               "response: 0x%" PRIx32 "\n",
               device_id,
               enable_scanning_response);
    }

    if (second != -1) {
        *result_out = (enum ps2_device_kind)(first << 8 | second);
        return true;
    }

    *result_out = (enum ps2_device_kind)first;
    return true;
}

static bool ps2_init_port(const enum ps2_port_id device_id) {
    const enum ps2_command enable_command =
        device_id == PS2_FIRST_PORT ?
            PS2_CMD_ENABLE_1ST_DEVICE : PS2_CMD_ENABLE_2ND_DEVICE;

    if (!ps2_send_command(enable_command)) {
        return false;
    }

    // Flush enable-command's response
    ps2_read_input_byte();

    const uint8_t reset_response =
        ps2_send_to_port(device_id, PS2_DATA_RESET_DEVICE);

    if (reset_response != PS2_RESPONSE_ACKNOWLEDGE) {
        printk(LOGLEVEL_WARN,
               "ps2: device %d did not acknowledge reset "
               "command, gave 0x%" PRIx8 "\n",
               device_id,
               reset_response);
        return false;
    }

    const uint8_t success_response = ps2_read_input_byte();
    if (success_response != PS2_RESPONSE_SUCCESS) {
        printk(LOGLEVEL_WARN,
               "ps2: device %d did not have a success response to reset "
               "command, gave 0x%" PRIx8 "\n",
               device_id,
               success_response);
        return false;
    }

    enum ps2_device_kind device_kind = PS2_DEVICE_KIND_MF2_KBD;
    if (!ps2_get_device_kind(device_id, &device_kind)) {
        return false;
    }

    switch (device_kind) {
        case PS2_DEVICE_KIND_MF2_KBD:
            printk(LOGLEVEL_INFO,
                   "ps2: got mf2 keyboard w/o translation. ignoring\n");
            return false;
        case PS2_DEVICE_KIND_STANDARD_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got standard mouse. ignoring\n");
            return false;
        case PS2_DEVICE_KIND_SCROLL_WHEEL_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got scroll wheel mouse. ignoring\n");
            return false;
        case PS2_DEVICE_KIND_5_BUTTON_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got 5 button mouse. ignoring\n");
            return false;
        case PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_1:
        case PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_2:
            printk(LOGLEVEL_INFO,
                   "ps2: got mf2 keyboard (with translation). Initializing\n");

            ps2_keyboard_init(device_id);
            return true;
    }

    printk(LOGLEVEL_WARN,
           "ps2: unrecognized device: 0x%" PRIx8 "\n",
           device_kind);

    return false;
}

void ps2_init() {
    if (!ps2_send_command(PS2_CMD_DISABLE_1ST_DEVICE)) {
        printk(LOGLEVEL_WARN, "ps2: failed to disable 1st device for init\n");
        return;
    }

    if (!ps2_send_command(PS2_CMD_DISABLE_2ND_DEVICE)) {
        printk(LOGLEVEL_WARN, "ps2: failed to disable 2nd device for init\n");
        return;
    }

    int16_t ps2_config = ps2_read_config();
    if (ps2_config == -1) {
        printk(LOGLEVEL_WARN, "ps2: failed to read config for init\n");
        return;
    }

    ps2_config |=
        __PS2_CNTRLR_CONFIG_1ST_PORT_INTERRUPT |
        __PS2_CNTRLR_CONFIG_1ST_PORT_TRANSLATION;

    const bool has_second_port =
        (ps2_config & __PS2_CNTRLR_CONFIG_2ND_PORT_CLOCK) != 0;

    if (has_second_port) {
        ps2_config |= __PS2_CNTRLR_CONFIG_2ND_PORT_INTERRUPT;
    } else {
        printk(LOGLEVEL_WARN, "ps2: second port missing\n");
    }

    if (!ps2_write_config(ps2_config)) {
        printk(LOGLEVEL_WARN, "ps2: failed to write updated config for init\n");
        return;
    }

    bool first_port_enabled = true;
    if (ps2_send_command(PS2_CMD_TEST_1ST_DEVICE) &&
        ps2_read_input_byte() == E_PS2_TEST_PORT_OK)
    {
        first_port_enabled = true;
    } else {
        printk(LOGLEVEL_WARN, "ps2: first port failed self-test\n");
    }

    bool second_port_enabled = false;
    if (has_second_port) {
        if (ps2_send_command(PS2_CMD_TEST_2ND_DEVICE) &&
            ps2_read_input_byte() == E_PS2_TEST_PORT_OK)
        {
            second_port_enabled = true;
        } else {
            printk(LOGLEVEL_WARN, "ps2: second port failed self-test\n");
        }
    }

    if (first_port_enabled) {
        if (ps2_init_port(PS2_FIRST_PORT)) {
            printk(LOGLEVEL_INFO, "ps2: first port enabled\n");
        }
    }

    if (second_port_enabled) {
        if (ps2_init_port(PS2_SECOND_PORT)) {
            printk(LOGLEVEL_INFO, "ps2: second port enabled\n");
        }
    }
}