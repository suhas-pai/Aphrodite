/*
 * kernel/arch/x86_64/dev/ps2/driver.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#define PS2_READ_PORT_FAIL -1

enum ps2_command {
    PS2_CMD_READ_INPUT_BUFFER_BYTE = 0x20,
    PS2_CMD_WRITE_INPUT_BUFFER_BYTE = 0x60,

    PS2_CMD_DISABLE_2ND_DEVICE = 0xA7,
    PS2_CMD_ENABLE_2ND_DEVICE = 0xA8,

    PS2_CMD_TEST_2ND_DEVICE = 0xA9,
    PS2_CMD_TEST_CONTROLLER = 0xAA,
    PS2_CMD_TEST_1ST_DEVICE = 0xAB,

    PS2_CMD_DIAGNOSTIC_DUMP = 0xAC,

    PS2_CMD_DISABLE_1ST_DEVICE = 0xAD,
    PS2_CMD_ENABLE_1ST_DEVICE = 0xAE,

    PS2_CMD_READ_CNTRLR_INPUT = 0xC0,

    PS2_CMD_COPY_1_TO_3_BITS_INPUT_TO_STATUS_4_TO_7_BITS = 0xC1,
    PS2_CMD_COPY_4_TO_7_BITS_INPUT_TO_STATUS_4_TO_7_BITS = 0xC2,

    PS2_CMD_READ_CNTRLR_OUTPUT = 0xD0,
    PS2_CMD_WRITE_NEXT_BYTE_TO_CNTRLR_OUTPUT = 0xD1,
    PS2_CMD_WRITE_NEXT_BYTE_TO_1ST_DEVICE_OUTPUT = 0xD2,
    PS2_CMD_WRITE_NEXT_BYTE_TO_2ND_DEVICE_OUTPUT = 0xD3,
    PS2_CMD_WRITE_NEXT_BYTE_TO_2ND_DEVICE_INPUT = 0xD4,

    PS2_CMD_IDENTIFY = 0xF2,
    PS2_CMD_ENABLE_SCANNING = 0xF4,
    PS2_CMD_DISABLE_SCANNING = 0xF5
};

enum ps2_data {
    PS2_DATA_RESET_DEVICE = 0xFF
};

enum ps2_response {
    PS2_RESPONSE_ACKNOWLEDGE = 0xFA,
    PS2_RESPONSE_FAIL = 0xFC,
    PS2_RESPONSE_SUCCESS = 0xAA,
    PS2_RESPONSE_RESEND = 0xFE,
};

enum ps2_test_controller_result {
    E_PS2_TEST_CNTRLR_OK = 0x55,
    E_PS2_TEST_CNTRLR_FAILED = PS2_RESPONSE_FAIL,
};

enum ps2_test_port_result {
    E_PS2_TEST_PORT_OK,
};

enum ps2_controller_config_byte_masks {
    __PS2_CNTRLR_CONFIG_1ST_PORT_INTERRUPT = (1ull << 0),
    __PS2_CNTRLR_CONFIG_2ND_PORT_INTERRUPT = (1ull << 1),

    __PS2_CNTRLR_CONFIG_1ST_PORT_CLOCK = (1ull << 4),
    __PS2_CNTRLR_CONFIG_2ND_PORT_CLOCK = (1ull << 5),

    __PS2_CNTRLR_CONFIG_1ST_PORT_TRANSLATION = (1ull << 6),
};

enum ps2_status_register_masks {
    __PS2_STATUS_REG_OUTPUT_BUFFER_FULL = (1ull << 0),
    __PS2_STATUS_REG_INPUT_BUFFER_FULL  = (1ull << 1),

    __PS2_STATUS_REG_TIME_OUT_ERROR = (1ull << 6),
    __PS2_STATUS_REG_PARITY_ERROR   = (1ull << 7)
};

enum ps2_device_kind {
    PS2_DEVICE_KIND_STANDARD_MOUSE,
    PS2_DEVICE_KIND_SCROLL_WHEEL_MOUSE = 0x03,
    PS2_DEVICE_KIND_5_BUTTON_MOUSE     = 0x05,

    PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_1 = (uint16_t)(0xAB << 8) | 0x41,
    PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_2 = (uint16_t)(0xAB << 8) | 0xC1,

    PS2_DEVICE_KIND_MF2_KBD = (uint16_t)(0xAB << 8) | 0x83
};

enum ps2_port_id {
    PS2_FIRST_PORT = 1,
    PS2_SECOND_PORT
};

void ps2_init();

int16_t ps2_read_input_byte();
int16_t ps2_read_config();

int16_t ps2_send_to_port(const enum ps2_port_id device, const uint8_t byte);