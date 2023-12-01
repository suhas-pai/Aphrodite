/*
 * kernel/src/dev/dtb/parse.c
 * © suhas pai
 */

#include "lib/adt/string.h"

#include "dev/printk.h"
#include "fdt/libfdt.h"
#include "mm/kmalloc.h"

#include "parse.h"

__optimize(3) static inline bool
parse_array_prop(const struct fdt_property *const fdt_prop,
                 const int prop_length,
                 const fdt32_t **const data_out,
                 uint32_t *const length_out)
{
    *data_out = (const fdt32_t *)(uint64_t)fdt_prop->data;
    *length_out = (uint32_t)prop_length / sizeof(fdt32_t);

    return true;
}

__optimize(3) static inline bool
parse_cell_pair(const fdt32_t **const iter_ptr,
                const fdt32_t *const end,
                const int cell_count,
                uint64_t *const result_out)
{
    const fdt32_t *const iter = *iter_ptr;
    if (iter + cell_count > end) {
        return false;
    }

    switch (cell_count) {
        case 0:
            *result_out = 0;
            return true;
        case 1:
            *result_out = fdt32_to_cpu(*iter);
            *iter_ptr = iter + cell_count;

            return true;
        case 2:
            *result_out = fdt64_to_cpu(*(const fdt64_t *)(uint64_t)iter);
            *iter_ptr = iter + cell_count;

            return true;
        case 3:
            *result_out =
                (uint64_t)fdt32_to_cpu(*iter) << 32 |
                (uint64_t)(fdt32_to_cpu(iter[1]) << 16 | fdt32_to_cpu(iter[2]));

            *iter_ptr = iter + cell_count;
            return true;
    }

    printk(LOGLEVEL_WARN,
           "devicetree: couldn't parse pairs with an invalid cell count of "
           "%" PRIu32 "\n",
           cell_count);

    return false;
}

static bool
parse_reg_pairs(const void *const dtb,
                const struct fdt_property *const fdt_prop,
                const int prop_length,
                const int parent_off,
                struct array *const array)
{
    const int addr_cells = fdt_address_cells(dtb, parent_off);
    const int size_cells = fdt_size_cells(dtb, parent_off);

    if (addr_cells < 0 || size_cells < 0) {
        return false;
    }

    const fdt32_t *data = NULL;
    uint32_t data_length = 0;

    if (!parse_array_prop(fdt_prop, prop_length, &data, &data_length)) {
        return false;
    }

    if (data_length == 0) {
        return true;
    }

    const fdt32_t *const data_end = data + data_length;
    const uint32_t entry_size = (uint32_t)(addr_cells + size_cells);

    if (entry_size == 0) {
        return false;
    }

    if ((data_length % entry_size) != 0) {
        return false;
    }

    const uint32_t entry_count = data_length / entry_size;
    array_reserve(array, entry_count);

    for (uint32_t i = 0; i != entry_count; i++) {
        struct devicetree_prop_reg_info info;
        if (!parse_cell_pair(&data, data_end, addr_cells, &info.address)) {
            return false;
        }

        if (!parse_cell_pair(&data, data_end, size_cells, &info.size)) {
            return false;
        }

        if (!array_append(array, &info)) {
            return false;
        }
    }

    return true;
}

static bool
parse_ranges_prop(const void *const dtb,
                  const struct fdt_property *const fdt_prop,
                  const int prop_length,
                  const int nodeoff,
                  const int parent_off,
                  struct array *const array)
{
    const int parent_addr_cells = fdt_address_cells(dtb, parent_off);
    const int child_addr_cells = fdt_address_cells(dtb, nodeoff);
    const int size_cells = fdt_size_cells(dtb, nodeoff);

    if (parent_addr_cells < 0 || child_addr_cells < 0 || size_cells < 0) {
        return false;
    }

    const fdt32_t *data = NULL;
    uint32_t data_length = 0;

    if (!parse_array_prop(fdt_prop, prop_length, &data, &data_length)) {
        return false;
    }

    if (data_length == 0) {
        return true;
    }

    const uint32_t entry_size =
        (uint32_t)(parent_addr_cells + child_addr_cells + size_cells);

    if (entry_size == 0) {
        return false;
    }

    if ((data_length % entry_size) != 0) {
        return false;
    }

    const uint32_t entry_count = data_length / entry_size;
    array_reserve(array, entry_count);

    const fdt32_t *const data_end = data + data_length;
    for (uint32_t i = 0; i != entry_count; i++) {
        struct devicetree_prop_range_info info;
        if (!parse_cell_pair(&data,
                             data_end,
                             child_addr_cells,
                             &info.child_bus_address))
        {
            return false;
        }

        if (!parse_cell_pair(&data,
                             data_end,
                             parent_addr_cells,
                             &info.parent_bus_address))
        {
            return false;
        }

        if (!parse_cell_pair(&data, data_end, size_cells, &info.size)) {
            return false;
        }

        if (!array_append(array, &info)) {
            return false;
        }
    }

    return true;
}

__optimize(3) static inline
struct string_view get_prop_data_sv(const struct fdt_property *const fdt_prop) {
    return
        sv_create_length((const char *)fdt_prop->data,
                         strnlen(fdt_prop->data, fdt_prop->len));
}

__optimize(3) static inline bool
parse_model_prop(const struct fdt_property *const fdt_prop,
                 struct string_view *const manufacturer_out,
                 struct string_view *const model_out)
{
    const struct string_view model_sv = get_prop_data_sv(fdt_prop);
    const int64_t comma_index = sv_find_char(model_sv, /*index=*/0, ',');

    if (comma_index == -1) {
        return false;
    }

    *manufacturer_out = sv_substring_upto(model_sv, comma_index);
    *model_out = sv_substring_from(model_sv, comma_index + 1);

    return true;
}

__optimize(3) static inline bool
parse_status_prop(const struct fdt_property *const fdt_prop,
                  enum devicetree_prop_status_kind *const kind_out)
{
    const struct string_view status_sv = get_prop_data_sv(fdt_prop);
    enum devicetree_prop_status_kind kind = DEVICETREE_PROP_STATUS_OKAY;

    switch (kind) {
        case DEVICETREE_PROP_STATUS_OKAY:
            if (sv_equals(status_sv, SV_STATIC("okay"))) {
                *kind_out = DEVICETREE_PROP_STATUS_OKAY;
                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_STATUS_DISABLED:
            if (sv_equals(status_sv, SV_STATIC("disabled"))) {
                *kind_out = DEVICETREE_PROP_STATUS_DISABLED;
                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_STATUS_RESERVED:
            if (sv_equals(status_sv, SV_STATIC("reserved"))) {
                *kind_out = DEVICETREE_PROP_STATUS_RESERVED;
                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_STATUS_FAIL:
            if (sv_equals(status_sv, SV_STATIC("fail"))) {
                *kind_out = DEVICETREE_PROP_STATUS_FAIL;
                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_STATUS_FAIL_SSS:
            if (sv_equals(status_sv, SV_STATIC("fail-sss"))) {
                *kind_out = DEVICETREE_PROP_STATUS_FAIL_SSS;
                return true;
            }

            break;
    }

    printk(LOGLEVEL_WARN,
           "devicetree: status prop has an unknown value: " SV_FMT "\n",
           SV_FMT_ARGS(status_sv));

    return false;
}

__optimize(3) static inline bool
parse_integer_prop(const struct fdt_property *const fdt_prop,
                   const int prop_length,
                   uint32_t *const int_out)
{
    if (prop_length != sizeof(fdt32_t)) {
        return false;
    }

    *int_out = fdt32_to_cpu(*(fdt32_t *)(uint64_t)fdt_prop->data);
    return true;
}

__optimize(3) static inline bool
parse_integer_list_prop(const struct fdt_property *const fdt_prop,
                        const int prop_length,
                        struct array *const array)
{
    const fdt32_t *data = NULL;
    uint32_t data_length = 0;

    if (!parse_array_prop(fdt_prop, prop_length, &data, &data_length)) {
        return false;
    }

    if (data_length == 0) {
        return true;
    }

    const fdt32_t *const num_list = (fdt32_t *)(uint64_t)fdt_prop->data;

    *array = ARRAY_INIT(sizeof(uint32_t));
    array_reserve(array, data_length);

    for (uint32_t i = 0; i != data_length; i++) {
        const uint32_t num = fdt32_to_cpu(num_list[i]);
        if (!array_append(array, &num)) {
            return false;
        }
    }

    return true;
}

__optimize(3) static inline bool
parse_range_prop(const struct fdt_property *const fdt_prop,
                 const int prop_length,
                 struct range *const range_out)
{
    if (prop_length != (sizeof(fdt32_t) * 2)) {
        return false;
    }

    fdt32_t *const data = (fdt32_t *)(uint64_t)fdt_prop->data;

    range_out->front = fdt32_to_cpu(data[0]);
    range_out->size = fdt32_to_cpu(data[1]);

    return true;
}

__optimize(3) static inline int
fdt_cells(const void *const fdt, const int nodeoffset, const char *const name) {
    const fdt32_t *c;
    uint32_t val;
    int len;

    c = fdt_getprop(fdt, nodeoffset, name, &len);
    if (!c)
        return len;

    if (len != sizeof(*c))
        return -FDT_ERR_BADNCELLS;

    val = fdt32_to_cpu(*c);
    if (val > FDT_MAX_NCELLS)
        return -FDT_ERR_BADNCELLS;

    return (int)val;
}

__optimize(3) static inline bool
parse_int_info(const fdt32_t **const data_ptr,
               const fdt32_t *const data_end,
               const uint32_t parent_int_cells,
               const uint32_t phandle,
               struct devicetree_prop_int_map_entry_int_info *const int_info)
{
    if (parent_int_cells != 3) {
        printk(LOGLEVEL_WARN,
               "devicetree: interrupt-map's phandle %" PRIu32 "'s "
               "corresponding node #interrupt-cells property doesn't have "
               "a value of 3\n",
               phandle);
        return false;
    }

    const fdt32_t *const data = *data_ptr;
    if (data + parent_int_cells > data_end) {
        return false;
    }

    int_info->is_ppi = fdt32_to_cpu(data[0]);
    int_info->flags = fdt32_to_cpu(data[2]);
    int_info->id = fdt32_to_cpu(data[1]) + (int_info->is_ppi ? 16 : 32);

    switch (int_info->flags & 0xF) {
        case 1:
            int_info->polarity = DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_HIGH;
            int_info->trigger_mode =
                DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_EDGE;

            *data_ptr = data + parent_int_cells;
            return true;
        case 2:
            int_info->polarity = DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_LOW;
            int_info->trigger_mode =
                DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_EDGE;

            *data_ptr = data + parent_int_cells;
            return true;
        case 4:
            int_info->polarity = DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_HIGH;
            int_info->trigger_mode =
                DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_LEVEL;

            *data_ptr = data + parent_int_cells;
            return true;
        case 8:
            int_info->polarity = DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_LOW;
            int_info->trigger_mode =
                DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_LEVEL;

            *data_ptr = data + parent_int_cells;
            return true;
    }

    printk(LOGLEVEL_WARN,
           "devicetree: unrecognized polarity/trigger-mode information of "
           "interrupt\n");

    return false;
}

static bool
parse_interrupt_map_prop(const void *const dtb,
                         const struct fdt_property *const fdt_prop,
                         const uint32_t prop_length,
                         const int nodeoff,
                         struct devicetree *const tree,
                         struct array *const array)
{
    const int child_unit_addr_cells = fdt_address_cells(dtb, nodeoff);
    const int child_int_cells = fdt_cells(dtb, nodeoff, "#interrupt-cells");

    if (child_unit_addr_cells < 0 || child_int_cells < 0) {
        return false;
    }

    if (child_int_cells > 2) {
        printk(LOGLEVEL_WARN,
               "devicetree: couldn't parse child-interrupt pair with an "
               "invalid #interrupt-cells value of %" PRIu32 "\n",
               child_int_cells);
        return false;
    }

    const fdt32_t *data = NULL;
    uint32_t data_length = 0;

    if (!parse_array_prop(fdt_prop, (int)prop_length, &data, &data_length)) {
        return false;
    }

    if (data_length == 0) {
        return true;
    }

    const fdt32_t *const data_end = data + data_length;
    while (data < data_end) {
        struct devicetree_prop_interrupt_map_entry info;
        if (!parse_cell_pair(&data,
                             data_end,
                             child_unit_addr_cells,
                             &info.child_unit_address))
        {
            return false;
        }

        if (!parse_cell_pair(&data,
                             data_end,
                             child_int_cells,
                             (uint64_t *)&info.child_int_specifier))
        {
            return false;
        }

        info.phandle = fdt32_to_cpu(*data);
        data++;

        struct devicetree_node *const phandle_node =
            devicetree_get_node_for_phandle(tree, info.phandle);

        if (phandle_node == NULL) {
            printk(LOGLEVEL_WARN,
                   "devicetree: interrupt-map refers to a phandle "
                   "0x%" PRIx32 " w/o a corresponding node\n",
                   info.phandle);
            return false;
        }

        const struct devicetree_prop_addr_size_cells *const phandle_prop_info =
            (const struct devicetree_prop_addr_size_cells *)
                devicetree_node_get_prop(phandle_node,
                                         DEVICETREE_PROP_ADDR_SIZE_CELLS);

        if (phandle_prop_info != NULL) {
            if (!parse_cell_pair(&data,
                                data_end,
                                (int)phandle_prop_info->addr_cells,
                                (uint64_t *)&info.child_int_specifier))
            {
                return false;
            }
        }

        const struct devicetree_prop_interrupt_cells *const int_cells_prop =
            (const struct devicetree_prop_interrupt_cells *)
                devicetree_node_get_prop(phandle_node,
                                         DEVICETREE_PROP_INTERRUPT_CELLS);

        if (int_cells_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "devicetree: interrupt-map's phandle 0x%" PRIx32 "'s "
                   "corresponding node is missing the #interrupt-cells "
                   "property\n",
                   info.phandle);
            return false;
        }

        const bool is_parent_int_controller =
            devicetree_node_get_prop(phandle_node,
                                     DEVICETREE_PROP_INTERRUPT_CONTROLLER)
            != NULL;

        if (!is_parent_int_controller) {
            printk(LOGLEVEL_WARN,
                   "devicetree: interrupt-map's phandle 0x%" PRIx32 "'s "
                   "corresponding node is missing the interrupt-controller "
                   "property\n",
                   info.phandle);
            return false;
        }

        if (!parse_int_info(&data,
                            data_end,
                            /*parent_int_cells=*/int_cells_prop->count,
                            info.phandle,
                            &info.parent_int_info))
        {
            return false;
        }

        if (!array_append(array, &info)) {
            return false;
        }
    }

    return true;
}

static bool
parse_specifier_map_prop(const void *const dtb,
                         const struct devicetree_node *const node,
                         const struct fdt_property *const fdt_prop,
                         const int prop_length,
                         const int parent_off,
                         struct array *const array)
{
    struct string cells_key = STRING_NULL();

    string_reserve(&cells_key, LEN_OF("#-cells") + node->name.length);
    string_append_char(&cells_key, '#', /*amt=*/1);

    string_append_sv(&cells_key, node->name);
    string_append_sv(&cells_key, SV_STATIC("-cells"));

    const int cells = fdt_cells(dtb, parent_off, string_to_cstr(cells_key));
    if (cells < 0) {
        // If we don't have a corresponding cells prop, then we assume this
        // isn't a specifier-map prop.

        return true;
    }

    const fdt32_t *data = NULL;
    uint32_t data_length = 0;

    if (!parse_array_prop(fdt_prop, prop_length, &data, &data_length)) {
        return false;
    }

    if (data_length == 0) {
        return true;
    }

    const uint32_t entry_size = (uint32_t)cells;
    if (entry_size == 0) {
        return false;
    }

    if ((data_length % entry_size) != 0) {
        return false;
    }

    const uint32_t entry_count = data_length / entry_size;
    array_reserve(array, entry_count);

    const fdt32_t *const data_end = data + data_length;
    for (uint32_t i = 0; i != entry_count; i++) {
        struct devicetree_prop_spec_map_entry info;
        if (!parse_cell_pair(&data, data_end, cells, &info.child_specifier)) {
            return false;
        }

        info.specifier_parent = fdt32_to_cpu(*data);
        data++;

        if (!parse_cell_pair(&data, data_end, cells, &info.parent_specifier)) {
            return false;
        }

        if (!array_append(array, &info)) {
            return false;
        }
    }

    return true;
}

struct int_map_info {
    const struct fdt_property *fdt_prop;
    struct devicetree_node *node;

    uint32_t prop_length;
};

static bool
parse_node_prop(const void *const dtb,
                const int nodeoff,
                const int prop_off,
                const int parent_nodeoff,
                struct devicetree *const tree,
                struct devicetree_node *const node,
                struct array *const int_map_list,
                struct devicetree_prop_addr_size_cells *const addr_size_cells)
{
    int prop_len = 0;
    int name_len = 0;

    const struct fdt_property *const fdt_prop =
        fdt_get_property_by_offset(dtb, prop_off, &prop_len);
    const char *const prop_string =
        fdt_get_string(dtb,
                       (int)fdt32_to_cpu((fdt32_t)fdt_prop->nameoff),
                       &name_len);

    const struct string_view name =
        sv_create_length(prop_string, (uint64_t)name_len);

    enum devicetree_prop_kind kind = DEVICETREE_PROP_COMPAT;
    switch (kind) {
        case DEVICETREE_PROP_COMPAT:
            if (sv_equals(name, SV_STATIC("compatible"))) {
                struct devicetree_prop_compat *const compat_prop =
                    kmalloc(sizeof(*compat_prop));

                if (compat_prop == NULL) {
                    return false;
                }

                compat_prop->kind = DEVICETREE_PROP_COMPAT;
                compat_prop->string = get_prop_data_sv(fdt_prop);

                if (!array_append(&node->known_props, &compat_prop)) {
                    kfree(compat_prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_REG:
            if (sv_equals(name, SV_STATIC("reg"))) {
                struct array list =
                    ARRAY_INIT(sizeof(struct devicetree_prop_reg_info));

                if (!parse_reg_pairs(dtb,
                                     fdt_prop,
                                     prop_len,
                                     parent_nodeoff,
                                     &list))
                {
                    array_destroy(&list);
                    return false;
                }

                struct devicetree_prop_reg *const prop = kmalloc(sizeof(*prop));
                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_REG;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_RANGES:
            if (sv_equals(name, SV_STATIC("ranges"))) {
                struct array list =
                    ARRAY_INIT(sizeof(struct devicetree_prop_range_info));

                if (!parse_ranges_prop(dtb,
                                       fdt_prop,
                                       prop_len,
                                       nodeoff,
                                       parent_nodeoff,
                                       &list))
                {
                    array_destroy(&list);
                    return false;
                }

                struct devicetree_prop_ranges *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_RANGES;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_MODEL:
            if (sv_equals(name, SV_STATIC("model"))) {
                struct string_view manufacturer_sv = SV_EMPTY();
                struct string_view model_sv = SV_EMPTY();

                if (!parse_model_prop(fdt_prop, &manufacturer_sv, &model_sv)) {
                    return false;
                }

                struct devicetree_prop_model *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_MODEL;
                prop->manufacturer = manufacturer_sv;
                prop->model = model_sv;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_STATUS:
            if (sv_equals(name, SV_STATIC("status"))) {
                enum devicetree_prop_status_kind status =
                    DEVICETREE_PROP_STATUS_OKAY;

                if (!parse_status_prop(fdt_prop, &status)) {
                    return false;
                }

                struct devicetree_prop_status *const status_prop =
                    kmalloc(sizeof(*status_prop));

                if (status_prop == NULL) {
                    return false;
                }

                status_prop->kind = DEVICETREE_PROP_STATUS;
                status_prop->status = status;

                if (!array_append(&node->known_props, &status_prop)) {
                    kfree(status_prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_ADDR_SIZE_CELLS:
            if (sv_equals(name, SV_STATIC("#address-cells"))) {
                uint32_t addr_cells = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &addr_cells)) {
                    return false;
                }

                if (addr_cells == 0) {
                    return false;
                }

                addr_size_cells->addr_cells = addr_cells;
                return true;
            } else if (sv_equals(name, SV_STATIC("#size-cells"))) {
                uint32_t size_cells = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &size_cells)) {
                    return false;
                }

                addr_size_cells->size_cells = size_cells;
                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_PHANDLE:
            if (sv_equals(name, SV_STATIC("phandle")) ||
                sv_equals(name, SV_STATIC("linux,phandle")))
            {
                uint32_t phandle = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &phandle)) {
                    return false;
                }

                struct devicetree_prop_phandle *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_PHANDLE;
                prop->phandle = phandle;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                if (!hashmap_add(&tree->phandle_map,
                                 hashmap_key_create(phandle),
                                 &node))
                {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_VIRTUAL_REG:
            if (sv_equals(name, SV_STATIC("virtual-reg"))) {
                uint32_t address = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &address)) {
                    return false;
                }

                struct devicetree_prop_virtual_reg *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_VIRTUAL_REG;
                prop->address = address;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_DMA_RANGES:
            if (sv_equals(name, SV_STATIC("dma-ranges"))) {
                struct array list =
                    ARRAY_INIT(sizeof(struct devicetree_prop_range_info));

                if (!parse_ranges_prop(dtb,
                                       fdt_prop,
                                       prop_len,
                                       nodeoff,
                                       parent_nodeoff,
                                       &list))
                {
                    array_destroy(&list);
                    return false;
                }

                struct devicetree_prop_ranges *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_DMA_RANGES;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_DMA_COHERENT:
            if (sv_equals(name, SV_STATIC("dma-coherent"))) {
                struct devicetree_prop_no_value *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_DMA_COHERENT;
                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_DEVICE_TYPE:
            if (sv_equals(name, SV_STATIC("device_type"))) {
                struct devicetree_prop_device_type *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_DEVICE_TYPE;
                prop->name = get_prop_data_sv(fdt_prop);;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_INTERRUPTS:
            if (sv_equals(name, SV_STATIC("interrupts"))) {
                struct array list = ARRAY_INIT(sizeof(uint32_t));
                if (!parse_integer_list_prop(fdt_prop, prop_len, &list)) {
                    array_destroy(&list);
                    return false;
                }

                struct devicetree_prop_interrupts *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_INTERRUPTS;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_INTERRUPT_MAP:
            if (sv_equals(name, SV_STATIC("interrupt-map"))) {
                const struct int_map_info info = {
                    .fdt_prop = fdt_prop,
                    .node = node,
                    .prop_length = (uint32_t)prop_len
                };

                if (!array_append(int_map_list, &info)) {
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_INTERRUPT_PARENT:
            if (sv_equals(name, SV_STATIC("interrupt-parent"))) {
                uint32_t phandle = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &phandle)) {
                    return false;
                }

                struct devicetree_prop_interrupt_parent *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_INTERRUPT_PARENT;
                prop->phandle = phandle;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];;
        case DEVICETREE_PROP_INTERRUPT_CONTROLLER:
            if (sv_equals(name, SV_STATIC("interrupt-controller"))) {
                struct devicetree_prop_no_value *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_INTERRUPT_CONTROLLER;
                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];;
        case DEVICETREE_PROP_INTERRUPT_CELLS:
            if (sv_equals(name, SV_STATIC("#interrupt-cells"))) {
                uint32_t count = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &count)) {
                    return false;
                }

                struct devicetree_prop_interrupt_cells *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_INTERRUPT_CELLS;
                prop->count = count;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_INTERRUPT_MAP_MASK:
            if (sv_equals(name, SV_STATIC("interrupt-map-mask"))) {
                struct array list = ARRAY_INIT(sizeof(uint32_t));
                if (!parse_integer_list_prop(fdt_prop, prop_len, &list)) {
                    array_destroy(&list);
                    return false;
                }

                struct devicetree_prop_interrupt_map_mask *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_INTERRUPT_MAP_MASK;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_MSI_CONTROLLER:
            if (sv_equals(name, SV_STATIC("msi-controller"))) {
                struct devicetree_prop_no_value *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_MSI_CONTROLLER;
                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_SPECIFIER_MAP:
            if (sv_has_suffix(name, SV_STATIC("-map"))) {
                struct array list =
                    ARRAY_INIT(sizeof(struct devicetree_prop_spec_map_entry));

                if (!parse_specifier_map_prop(dtb,
                                              node,
                                              fdt_prop,
                                              prop_len,
                                              parent_nodeoff,
                                              &list))
                {
                    array_destroy(&list);
                    return false;
                }

                if (array_empty(list)) {
                    break;
                }

                struct devicetree_prop_specifier_map *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    array_destroy(&list);
                    return false;
                }

                prop->kind = DEVICETREE_PROP_SPECIFIER_MAP;
                prop->name = name;
                prop->list = list;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    array_destroy(&list);

                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_SPECIFIER_CELLS:
            if (sv_has_suffix(name, SV_STATIC("-cells"))) {
                uint32_t cells = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &cells)) {
                    return false;
                }

                struct devicetree_prop_specifier_cells *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_SPECIFIER_CELLS;
                prop->name = name;
                prop->cells = cells;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_SERIAL_CLOCK_FREQ:
            if (sv_equals(name, SV_STATIC("clock-frequency"))) {
                uint32_t frequency = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &frequency)) {
                    return false;
                }

                struct devicetree_prop_clock_frequency *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_SERIAL_CLOCK_FREQ;
                prop->frequency = frequency;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_SERIAL_CURRENT_SPEED:
            if (sv_equals(name, SV_STATIC("current-speed"))) {
                uint32_t speed = 0;
                if (!parse_integer_prop(fdt_prop, prop_len, &speed)) {
                    return false;
                }

                struct devicetree_prop_current_speed *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_SERIAL_CURRENT_SPEED;
                prop->speed = speed;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            [[fallthrough]];
        case DEVICETREE_PROP_PCI_BUS_RANGE:
            if (sv_equals(name, SV_STATIC("bus-range"))) {
                struct range bus_range = RANGE_EMPTY();
                if (!parse_range_prop(fdt_prop, prop_len, &bus_range)) {
                    return false;
                }

                struct devicetree_prop_bus_range *const prop =
                    kmalloc(sizeof(*prop));

                if (prop == NULL) {
                    return false;
                }

                prop->kind = DEVICETREE_PROP_PCI_BUS_RANGE;
                prop->range = bus_range;

                if (!array_append(&node->known_props, &prop)) {
                    kfree(prop);
                    return false;
                }

                return true;
            }

            break;
    }

    struct devicetree_prop_other *const other_prop =
        kmalloc(sizeof(*other_prop));

    if (other_prop == NULL) {
        return false;
    }

    other_prop->name = name;
    other_prop->data = (const fdt32_t *)(uint64_t)fdt_prop->data;
    other_prop->data_length = (uint32_t)prop_len;

    if (!array_append(&node->other_props, &other_prop)) {
        kfree(other_prop);
        return false;
    }

    return true;
}

static bool
parse_node_children(const void *const dtb,
                    struct devicetree *const tree,
                    struct devicetree_node *const parent,
                    struct array *const int_map_list)
{
    int nodeoff = 0;
    fdt_for_each_subnode(nodeoff, dtb, parent->nodeoff) {
        struct devicetree_node *const node = kmalloc(sizeof(*node));
        if (node == NULL) {
            return false;
        }

        list_init(&node->child_list);
        list_init(&node->sibling_list);

        int lenp = 0;

        node->parent = parent;
        node->nodeoff = nodeoff;
        node->known_props = ARRAY_INIT(sizeof(struct devicetree_prop *));
        node->other_props = ARRAY_INIT(sizeof(struct devicetree_prop_other *));
        node->name =
            sv_create_length(fdt_get_name(dtb, nodeoff, &lenp), (uint64_t)lenp);

        struct devicetree_prop_addr_size_cells addr_size_cells_prop = {
            .kind = DEVICETREE_PROP_ADDR_SIZE_CELLS,
            .addr_cells = UINT32_MAX,
            .size_cells = UINT32_MAX
        };

        int prop_off = 0;
        fdt_for_each_property_offset(prop_off, dtb, nodeoff) {
            if (!parse_node_prop(dtb,
                                 nodeoff,
                                 prop_off,
                                 parent->nodeoff,
                                 tree,
                                 node,
                                 int_map_list,
                                 &addr_size_cells_prop))
            {
                return false;
            }
        }

        if (addr_size_cells_prop.addr_cells != UINT32_MAX) {
            if (addr_size_cells_prop.size_cells == UINT32_MAX) {
                printk(LOGLEVEL_WARN,
                       "devicetree: node " SV_FMT " has #address-cells prop "
                       "but no #size-cells prop\n",
                       SV_FMT_ARGS(node->name));
                return false;
            }

            struct devicetree_prop_addr_size_cells *const addr_size_cells =
                kmalloc(sizeof(*addr_size_cells));

            *addr_size_cells = addr_size_cells_prop;
            if (!array_append(&node->known_props, &addr_size_cells)) {
                kfree(addr_size_cells);
                return false;
            }
        } else if (addr_size_cells_prop.size_cells != UINT32_MAX) {
            printk(LOGLEVEL_WARN,
                   "devicetree: node " SV_FMT " has #size-cells prop but no "
                   "#address-cells prop\n",
                   SV_FMT_ARGS(node->name));
            return false;
        }

        if (!parse_node_children(dtb, tree, node, int_map_list)) {
            return false;
        }

        list_add(&parent->child_list, &node->sibling_list);
    }

    return true;
}

bool devicetree_parse(struct devicetree *const tree, const void *const dtb) {
    int prop_off = 0;

    struct array int_map_list = ARRAY_INIT(sizeof(struct int_map_info));
    struct devicetree_prop_addr_size_cells addr_size_cells_prop;
    struct devicetree_node *const root = tree->root;

    fdt_for_each_property_offset(prop_off, dtb, /*nodeoffset=*/0) {
        if (!parse_node_prop(dtb,
                             /*nodeoff=*/0,
                             prop_off,
                             /*parent_nodeoff=*/-1,
                             tree,
                             root,
                             &int_map_list,
                             &addr_size_cells_prop))
        {
            array_destroy(&int_map_list);
            devicetree_node_free(tree->root);

            return false;
        }
    }

    if (!parse_node_children(dtb, tree, tree->root, &int_map_list)) {
        array_destroy(&int_map_list);
        devicetree_node_free(tree->root);

        return false;
    }

    array_foreach(&int_map_list, struct int_map_info, iter) {
        struct array list =
            ARRAY_INIT(sizeof(struct devicetree_prop_interrupt_map_entry));

        if (!parse_interrupt_map_prop(dtb,
                                      iter->fdt_prop,
                                      iter->prop_length,
                                      iter->node->nodeoff,
                                      tree,
                                      &list))
        {
            array_destroy(&list);
            array_destroy(&int_map_list);

            devicetree_node_free(tree->root);
            return false;
        }

        struct devicetree_prop_interrupt_map *const map_prop =
            kmalloc(sizeof(*map_prop));

        if (map_prop == NULL) {
            array_destroy(&list);
            array_destroy(&int_map_list);

            devicetree_node_free(tree->root);
            return false;
        }

        map_prop->kind = DEVICETREE_PROP_INTERRUPT_MAP;
        map_prop->list = list;

        if (!array_append(&iter->node->known_props, &map_prop)) {
            array_destroy(&list);
            array_destroy(&int_map_list);

            devicetree_node_free(tree->root);
            kfree(map_prop);

            return false;
        }
    }

    array_destroy(&int_map_list);
    return true;
}