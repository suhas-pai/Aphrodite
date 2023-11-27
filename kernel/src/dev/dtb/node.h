/*
 * kernel/src/dev/dtb/node.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "lib/adt/string_view.h"

#include "lib/list.h"

enum devicetree_prop_kind {
    DEVICETREE_PROP_COMPAT,
    DEVICETREE_PROP_REG,
    DEVICETREE_PROP_RANGES,
    DEVICETREE_PROP_MODEL,
    DEVICETREE_PROP_STATUS,
    DEVICETREE_PROP_ADDR_SIZE_CELLS,
    DEVICETREE_PROP_PHANDLE,
    DEVICETREE_PROP_VIRTUAL_REG,
    DEVICETREE_PROP_DMA_RANGES,
    DEVICETREE_PROP_DMA_COHERENT,
    DEVICETREE_PROP_INTERRUPTS,
    DEVICETREE_PROP_INTERRUPT_MAP,
    DEVICETREE_PROP_INTERRUPT_PARENT,
    DEVICETREE_PROP_INTERRUPT_CELLS,
    DEVICETREE_PROP_INTERRUPT_MAP_MASK,
    DEVICETREE_PROP_SPECIFIER_MAP,
    DEVICETREE_PROP_SPECIFIER_CELLS,
};

struct devicetree_prop {
    enum devicetree_prop_kind kind;
};

struct devicetree_prop_compat {
    enum devicetree_prop_kind kind;
    struct string_view string;
};

struct devicetree_reg_addr_size_pair {
    uint64_t address;
    uint64_t size;
};

struct devicetree_prop_reg_info {
    uint64_t address;
    uint64_t size;
};

struct devicetree_prop_reg {
    enum devicetree_prop_kind kind;
    struct array list;
};

struct devicetree_prop_range_info {
    uint64_t child_bus_address;
    uint64_t parent_bus_address;
    uint64_t size;
};

struct devicetree_prop_ranges {
    enum devicetree_prop_kind kind;
    struct array list;
};

struct devicetree_prop_model {
    enum devicetree_prop_kind kind;

    struct string_view manufacturer;
    struct string_view model;
};

struct devicetree_prop_phandle {
    enum devicetree_prop_kind kind;
    uint32_t phandle;
};

struct devicetree_prop_virtual_reg {
    enum devicetree_prop_kind kind;
    uint32_t address;
};

enum devicetree_prop_status_kind {
    DEVICETREE_PROP_STATUS_OKAY,
    DEVICETREE_PROP_STATUS_DISABLED,
    DEVICETREE_PROP_STATUS_RESERVED,
    DEVICETREE_PROP_STATUS_FAIL,
    DEVICETREE_PROP_STATUS_FAIL_SSS,
};

struct devicetree_prop_status {
    enum devicetree_prop_kind kind;
    enum devicetree_prop_status_kind status;
};

struct devicetree_prop_addr_size_cells {
    enum devicetree_prop_kind kind;

    uint32_t addr_cells;
    uint32_t size_cells;
};

struct devicetree_prop_no_value {
    enum devicetree_prop_kind kind;
};

struct devicetree_prop_interrupts {
    enum devicetree_prop_kind kind;
    struct array list;
};

struct devicetree_prop_interrupt_parent {
    enum devicetree_prop_kind kind;
    uint32_t phandle;
};

struct devicetree_prop_interrupt_cells {
    enum devicetree_prop_kind kind;
    uint32_t count;
};

enum devicetree_prop_int_polarity {
    DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_LOW,
    DEVTREE_PROP_INT_MAP_INT_ENTRY_POLARITY_HIGH,
};

enum devicetree_prop_int_trigger_mode {
    DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_EDGE,
    DEVTREE_PROP_INT_MAP_INT_ENTRY_TRIGGER_MODE_LEVEL,
};

struct devicetree_prop_int_map_entry_int_info {
    uint32_t id;
    uint32_t flags;

    bool is_ppi : 1;

    enum devicetree_prop_int_polarity polarity : 1;
    enum devicetree_prop_int_trigger_mode trigger_mode : 1;
};

struct devicetree_prop_interrupt_map_entry {
    uint64_t child_unit_address;
    uint32_t child_int_specifier;
    uint32_t phandle;
    uint64_t parent_unit_address;

    struct devicetree_prop_int_map_entry_int_info parent_int_info;
};

struct devicetree_prop_interrupt_map {
    enum devicetree_prop_kind kind;
    struct array list;
};

struct devicetree_prop_interrupt_map_mask {
    enum devicetree_prop_kind kind;
    uint32_t mask;
};

struct devicetree_prop_specifier_map_entry {
    uint64_t child_specifier;
    uint32_t specifier_parent;
    uint64_t parent_specifier;
};

struct devicetree_prop_specifier_map {
    enum devicetree_prop_kind kind;

    struct string_view name;
    struct array list;
};

struct devicetree_prop_specifier_cells {
    enum devicetree_prop_kind kind;

    struct string_view name;
    uint32_t cells;
};

struct devicetree_prop_other {
    struct string_view name;

    const void *data;
    uint32_t data_length;
};

struct devicetree_node {
    struct string_view name;
    struct devicetree_node *parent;

    int nodeoff;

    struct list child_list;
    struct list sibling_list;

    struct array known_props;
    struct array other_props;
};

struct devicetree_prop *
devicetree_node_get_prop(struct devicetree_node *node,
                         enum devicetree_prop_kind kind);

struct devicetree_prop_other *
devicetree_node_get_other_prop(struct devicetree_node *node,
                               struct string_view sv);

void devicetree_node_free(struct devicetree_node *const node);
