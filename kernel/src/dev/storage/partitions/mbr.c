/*
 * kernel/src/dev/storage/partitions/mbr.c
 * © suhas pai
 */

#include "dev/storage/partitions/mbr.h"
#include "lib/macros.h"

__debug_optimize(3)
bool verify_mbr_header(const struct mbr_header *const header) {
    if (header->magic != 0) {
        return header->magic == MBR_HEADER_MAGIC;
    }

    return true;
}

__debug_optimize(3) bool verify_mbr_entry(const struct mbr_entry *const entry) {
    return entry->os_type != 0;
}