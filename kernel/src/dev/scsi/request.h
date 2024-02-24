/*
 * kernel/src/dev/scsi/request.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum scsi_command {
    SCSI_CMD_IDENTIFY,
    SCSI_CMD_READ,
    SCSI_CMD_WRITE
};

struct scsi_request {
    enum scsi_command command;
    union {
        struct {
            uint32_t position;
            uint32_t length;
        } read;
        struct {
            uint32_t position;
            uint32_t length;
        } write;
    };
};

#define SCSI_REQUEST_IDENTITY() \
    ((struct scsi_request){ .command = SCSI_CMD_IDENTIFY })

#define SCSI_REQUEST_READ(pos, len) \
    ((struct scsi_request){ \
        .command = SCSI_CMD_READ, \
        .read.position = (pos), \
        .read.length = (len) \
    })

#define SCSI_REQUEST_WRITE(pos, len) \
    ((struct scsi_request){ \
        .command = SCSI_CMD_WRITE, \
        .read.position = (pos), \
        .read.length = (len) \
    })
