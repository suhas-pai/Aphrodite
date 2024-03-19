/*
 * kernel/src/dev/storage/partitions/mbr.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "mbr.h"

__optimize(3) bool verify_mbr_header(const struct mbr_header *const header) {
    if (header->magic != 0) {
        return header->magic == MBR_HEADER_MAGIC;
    }

    return true;
}

__optimize(3) bool verify_mbr_entry(const struct mbr_entry *const entry) {
    return entry->os_type != 0;
}