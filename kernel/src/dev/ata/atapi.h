/*
 * kernel/src/dev/ata/atapi.h
 * © suhas pai
 */

#pragma once

enum atapi_command {
    ATAPI_CMD_TEST_UNIT_READY = 0x00,
    ATAPI_CMD_REQUEST_SENSE = 0x03,
    ATAPI_CMD_FORMAT_UNIT = 0x04,
    ATAPI_CMD_INQUIRY = 0x12,

    // (Eject device)
    ATAPI_CMD_START_STOP_UNIT = 0x1B,
    ATAPI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
    ATAPI_CMD_READ_FORMAT_CAPACITIES = 0x23,
    ATAPI_CMD_READ_CAPACITY = 0x25,
    ATAPI_CMD_READ_10 = 0x28,
    ATAPI_CMD_WRITE_10 = 0x2A,
    ATAPI_CMD_SEEK_10 = 0x2B,
    ATAPI_CMD_WRITE_AND_VERIFY_10 = 0x2E,
    ATAPI_CMD_VERIFY = 0x2F,
    ATAPI_CMD_SYNCHRONIZE_CACHE = 0x35,
    ATAPI_CMD_WRITE_BUFFER = 0x3B,
    ATAPI_CMD_READ_BUFFER = 0x3C,
    ATAPI_CMD_READ_TOC = 0x43,
    ATAPI_CMD_GET_CONFIGURATION = 0x46,
    ATAPI_CMD_GET_EVENT_STATUS_NOTIFICATION = 0x4A,
    ATAPI_CMD_READ_DISC_INFORMATION = 0x51,
    ATAPI_CMD_READ_TRACK_INFORMATION = 0x52,
    ATAPI_CMD_RESERVE_TRACK = 0x53,
    ATAPI_CMD_SEND_OPC_INFORMATION = 0x54,
    ATAPI_CMD_MODE_SELECT = 0x55,
    ATAPI_CMD_REPAIR_TRACK = 0x58,
    ATAPI_CMD_MODE_SENSE = 0x5A,
    ATAPI_CMD_CLOSE_TRACK_SESSION = 0x5B,
    ATAPI_CMD_READ_BUFFER_CAPACITY = 0x5C,
    ATAPI_CMD_SEND_CUE_SHEET = 0x5D,
    ATAPI_CMD_REPORT_LUNS = 0xA0,
    ATAPI_CMD_BLANK = 0xA1,
    ATAPI_CMD_SECURITY_PROTOCOL_IN = 0xA2,
    ATAPI_CMD_SEND_KEY = 0xA3,
    ATAPI_CMD_REPORT_KEY = 0xA4,
    ATAPI_CMD_LOAD_UNLOAD_MEDIUM = 0xA6,
    ATAPI_CMD_SET_READ_AHEAD    = 0xA7,
    ATAPI_CMD_READ =    0xA8,
    ATAPI_CMD_WRITE = 0xAA,
    ATAPI_CMD_READ_MEDIA_SERIAL_NUMBER = 0xAB,
    ATAPI_CMD_GET_PERFORMANCE = 0xAC,
    ATAPI_CMD_READ_DISC_STRUCTURE = 0xAD,
    ATAPI_CMD_SECURITY_PROTOCOL_OUT = 0xB5,
    ATAPI_CMD_SET_STREAMING = 0xB6,
    ATAPI_CMD_READ_CD_MSF = 0xB9,
    ATAPI_CMD_SET_CD_SPEED = 0xBB,
    ATAPI_CMD_MECHANISM_STATUS = 0xBD,
    ATAPI_CMD_READ_CD = 0xBE,
    ATAPI_CMD_SEND_DISC_STRUCTURE = 0xBF,
};