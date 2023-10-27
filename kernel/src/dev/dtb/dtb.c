/*
 * kernel/dev/dtb/dtb.c
 * © suhas pai
 */

#include "fdt/libfdt.h"
#include "dtb.h"

bool
dtb_get_string_prop(const void *const dtb,
                    const int nodeoff,
                    const char *const key,
                    struct string_view *const sv_out)
{
    int name_length_max = 0;
    const struct fdt_property *const prop =
        fdt_get_property(dtb, nodeoff, key, &name_length_max);

    if (prop == NULL) {
        return false;
    }

    const uint64_t length = strnlen(prop->data, (uint64_t)name_length_max);
    *sv_out = sv_create_length(prop->data, length);

    return true;
}

bool
dtb_get_array_prop(const void *const dtb,
                   const int nodeoff,
                   const char *const key,
                   const fdt32_t **const data_out,
                   uint32_t *const length_out)
{
    int length = 0;
    const struct fdt_property *const prop =
        fdt_get_property(dtb, nodeoff, key, &length);

    if (prop == NULL) {
        return false;
    }

    *data_out = (const fdt32_t *)(uint64_t)prop->data;
    *length_out = (uint32_t)length / sizeof(fdt32_t);

    return true;
}

bool
dtb_get_reg_pairs(const void *const dtb,
                  const int nodeoff,
                  const uint32_t start_index,
                  uint32_t *const entry_count_in,
                  struct dtb_addr_size_pair *const pairs_out)
{
    const int parent_off = fdt_parent_offset(dtb, nodeoff);
    if (parent_off < 0) {
        return false;
    }

    const int addr_cells = fdt_address_cells(dtb, parent_off);
    const int size_cells = fdt_size_cells(dtb, parent_off);

    if (addr_cells < 0 || size_cells < 0) {
        return false;
    }

    const fdt32_t *reg = NULL;
    uint32_t reg_length = 0;

    if (!dtb_get_array_prop(dtb, nodeoff, /*key=*/"reg", &reg, &reg_length)) {
        return false;
    }

    if (reg_length == 0) {
        return false;
    }

    if ((reg_length % (uint32_t)(addr_cells + size_cells)) != 0) {
        return false;
    }

    const fdt32_t *const reg_end = reg + reg_length;
    const fdt32_t *reg_iter =
        reg + ((uint32_t)(addr_cells + size_cells) * start_index);

    if (reg_iter >= reg_end) {
        return false;
    }

    const uint32_t entry_spaces = *entry_count_in;
    const uint32_t addr_shift = sizeof_bits(uint64_t) / (uint32_t)addr_cells;
    const uint32_t size_shift = sizeof_bits(uint64_t) / (uint32_t)size_cells;

    struct dtb_addr_size_pair *entry = pairs_out;
    const struct dtb_addr_size_pair *const end = &pairs_out[entry_spaces];

    for (; entry != end; entry++) {
        if (addr_shift != sizeof_bits(uint64_t)) {
            for (int i = 0; i != addr_cells; i++) {
                entry->address =
                    entry->address << addr_shift | fdt32_to_cpu(*reg_iter);
                reg_iter++;
            }
        } else {
            entry->address = fdt32_to_cpu(*reg_iter);
            reg_iter++;
        }

        if (size_shift != sizeof_bits(uint64_t)) {
            for (int i = 0; i != size_cells; i++) {
                entry->size =
                    entry->size << size_shift | fdt32_to_cpu(*reg_iter);
                reg_iter++;
            }
        } else {
            entry->size = fdt32_to_cpu(*reg_iter);
            reg_iter++;
        }

        if (reg_iter == reg_end) {
            *entry_count_in = (uint32_t)((entry + 1) - pairs_out);
            return true;
        }
    }

    *entry_count_in = (uint32_t)(entry - pairs_out);
    return true;
}