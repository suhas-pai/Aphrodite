/*
 * kernel/src/dev/ata/atapi.c
 * © suhas pai
 */

#include "atapi.h"

__optimize(3) const char *atapi_sense_to_cstr(const enum atapi_sense sense) {
    switch (sense) {
        case ATAPI_SENSE_NONE:
            return "none";
        case ATAPI_SENSE_RECOVERED_ERROR:
            return "recovered error";
        case ATAPI_SENSE_NOT_READY:
            return "not ready";
        case ATAPI_SENSE_MEDIUM_ERROR:
            return "medium error";
        case ATAPI_SENSE_HARDWARE_ERROR:
            return "hardware error";
        case ATAPI_SENSE_ILLEGAL_REQUEST:
            return "illegal request";
        case ATAPI_SENSE_UNIT_ATTENTION:
            return "unit-attention";
        case ATAPI_SENSE_DATA_PROTECT:
            return "data-protect";
        case ATAPI_SENSE_BLANK_CHECK:
            return "blank check";
        case ATAPI_SENSE_COPY_ABORTED:
            return "copy aborted";
        case ATAPI_SENSE_ABORTED_COMMAND:
            return "aborted command";
        case ATAPI_SENSE_VOLUME_OVERFLOW:
            return "volume overflow";
        case ATAPI_SENSE_MISCOMPARE:
            return "miscompare";
    }

    return "<unknown>";
}