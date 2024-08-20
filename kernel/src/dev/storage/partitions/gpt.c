/*
 * kernel/src/dev/storage/partitions/gpt.c
 * © suhas pai
 */

#include "lib/adt/string_view.h"
#include "gpt.h"

__debug_optimize(3)
bool verify_gpt_header(const struct gpt_header *const header) {
    if (!sv_equals_c_str(GPT_HEADER_MAGIC_SV, header->signature)) {
        return false;
    }

    if (header->length < GPT_HEADER_MIN_SIZE) {
        return false;
    }

    if (header->partition_entry_size != sizeof(struct gpt_entry)) {
        return false;
    }

    if (header->header_lba != 1) {
        return false;
    }

    if (header->first_usable_lba > header->last_usable_lba) {
        return false;
    }

    if (header->revision != GPT_HEADER_REVISION) {
        return false;
    }

    return true;
}

__debug_optimize(3)
bool gpt_entry_mount_ok(const struct gpt_entry *const entry) {
    if (entry->last < entry->start) {
        return false;
    }

    if (entry->guid_low64 == 0 && entry->guid_hi64 == 0) {
        return false;
    }

    const uint64_t mask =
        __GPT_ENTRY_ATTR_REQ_TO_FUNCTION | __GPT_ENTRY_ATTR_USED_BY_OS;

    if (entry->attributes & mask) {
        return false;
    }

    return true;
}