/*
 * kernel/include/acpi/resources.h
 * © suhas pai
 */

#pragma once
#include <uacpi/types.h>

#include "lib/adt/array.h"
#include "mm/simple_alloc.h"
#include "sys/irq.h"

struct os_acpi_irq_connection_info {
    enum irq_polarity polarity : 1;
    enum irq_trigger_mode trigger_mode : 1;

    bool is_shared : 1;
    bool wake_capable : 1;
};

struct os_acpi_irq_info {
    struct os_acpi_irq_connection_info;

    uint32_t irq_count;
    uint32_t *irq_list;
};

enum os_acpi_dma_direction : uint8_t {
    os_acpi_DMA_DIRECTION_FROM_DEVICE_TO_HOST,
    os_acpi_DMA_DIRECTION_FROM_HOST_TO_DEVICE,
};

enum os_acpi_dma_transfer_type : uint8_t {
    os_acpi_DMA_TRANSFER_TYPE_8_BIT,
    os_acpi_DMA_TRANSFER_TYPE_8_AND_16_BIT,
    os_acpi_DMA_TRANSFER_TYPE_16_BIT,
};

enum os_acpi_dma_channel_speed : uint8_t {
    os_acpi_DMA_CHANNEL_TYPE_COMPAT,

    os_acpi_DMA_CHANNEL_TYPE_A,
    os_acpi_DMA_CHANNEL_TYPE_B,
    os_acpi_DMA_CHANNEL_TYPE_F,
};

enum os_acpi_dma_transfer_width : uint8_t {
    os_acpi_DMA_TRANSFER_WIDTH_8_BIT,
    os_acpi_DMA_TRANSFER_WIDTH_16_BIT,
    os_acpi_DMA_TRANSFER_WIDTH_32_BIT,
    os_acpi_DMA_TRANSFER_WIDTH_64_BIT,
    os_acpi_DMA_TRANSFER_WIDTH_128_BIT,
    os_acpi_DMA_TRANSFER_WIDTH_256_BIT,
};

struct os_acpi_dma_info {
    enum os_acpi_dma_direction direction : 1;
    enum os_acpi_dma_transfer_type transfer_type : 2;
    enum os_acpi_dma_channel_speed channel_speed : 2;
    enum os_acpi_dma_transfer_width transfer_width : 3;

    bool is_bus_master : 1;

    uint32_t channel_count;
    uint8_t *channel_list;
};

struct os_acpi_fixed_dma_info {
    uint16_t request_line;
    uint16_t channel;

    uint8_t transfer_width;
};

enum os_acpi_io_decode_kind {
    OS_ACPI_IO_DECODE_KIND_16_BIT,
    OS_ACPI_IO_DECODE_KIND_10_BIT,
};

struct os_acpi_io_info {
    enum os_acpi_io_decode_kind decode_kind : 1;

    uint16_t minimum;
    uint16_t maximum;

    uint8_t alignment;
    uint8_t length;
};

struct os_acpi_fixed_io_info {
    uint16_t address;
    uint8_t length;
};

enum os_acpi_cache_kind : uint8_t {
    os_acpi_CACHE_KIND_NONE,
    os_acpi_CACHE_KIND_CACHEABLE,
    os_acpi_CACHE_KIND_WRITE_COMBINING,
    os_acpi_CACHE_KIND_PREFETCHABLE
};

enum os_acpi_range_mem_kind : uint8_t {
    os_acpi_RANGE_MEMKIND_MEMORY,
    os_acpi_RANGE_MEMKIND_RESERVED,
    os_acpi_RANGE_MEMKIND_os_acpi,
    os_acpi_RANGE_MEMKIND_NVS,
};

enum os_acpi_range_kind : uint8_t {
    os_acpi_RANGE_KIND_MEMORY,
    os_acpi_RANGE_KIND_IO,
    os_acpi_RANGE_KIND_BUS,
};

enum os_acpi_io_mem_kind : uint8_t {
    os_acpi_IO_MEM_TRANSLATION,
    os_acpi_IO_MEM_STATIC,
};

enum os_acpi_translation_kind : uint8_t {
    os_acpi_TRANSLATION_KIND_DENSE,
    os_acpi_TRANSLATION_KIND_SPARSE,
};

enum os_acpi_resource_direction : uint8_t {
    os_acpi_RESOURCE_DIRECTION_PRODUCER,
    os_acpi_RESOURCE_DIRECTION_CONSUMER,
};

struct os_acpi_memory_attribute {
    enum os_acpi_cache_kind cache_kind : 2;
    enum os_acpi_range_kind range_kind : 2;
    enum os_acpi_translation_kind translation_kind : 1;

    bool writable : 1;
};

struct os_acpi_io_attribute {
    enum os_acpi_range_kind range_kind : 2;
    enum os_acpi_io_mem_kind mem_kind : 1;
    enum os_acpi_translation_kind translation_kind : 1;

    bool writable : 1;
};

struct os_acpi_address_attribute {
    struct os_acpi_memory_attribute mem_attr;
    struct os_acpi_io_attribute io_attr;

    uint8_t type_specific;
};

struct os_acpi_resource_source {
    uint8_t index;
    bool index_valid : 1;

    uint16_t length;
    char *string;
};

struct os_acpi_resource_address_common {
    struct os_acpi_address_attribute attribute;
    uint8_t type;

    enum os_acpi_resource_direction direction : 1;
    enum os_acpi_io_decode_kind decode_kind : 1;

    uint8_t fixed_min_address;
    uint8_t fixed_max_address;
};

struct os_acpi_resource_address_u16 {
    struct os_acpi_resource_address_common;
    uint16_t granularity;

    uint16_t minimum;
    uint16_t maximum;

    uint16_t translation_offset;
    uint16_t address_length;

    struct os_acpi_resource_source source;
};

struct os_acpi_resource_address_u32 {
    struct os_acpi_resource_address_common;
    uint32_t granularity;

    uint32_t minimum;
    uint32_t maximum;

    uint32_t translation_offset;
    uint32_t address_length;

    struct os_acpi_resource_source source;
};

struct os_acpi_resource_address_u64 {
    struct os_acpi_resource_address_common;
    uint64_t granularity;

    uint64_t minimum;
    uint64_t maximum;

    uint64_t translation_offset;
    uint64_t address_length;

    struct os_acpi_resource_source source;
};

struct os_acpi_resource_address_u64_extended {
    struct os_acpi_resource_address_common;
    uint64_t granularity;

    uint64_t minimum;
    uint64_t maximum;

    uint64_t translation_offset;
    uint64_t address_length;

    uint64_t attributes;
};

struct os_acpi_resource_memory_info {
    uint32_t minimum;
    uint32_t maximum;
    uint32_t alignment;
    uint32_t length;

    bool writable : 1;
};

struct os_acpi_resource_fixed_memory_info {
    uint32_t address;
    uint32_t length;

    bool writable : 1;
};

enum os_acpi_resource_compat_perf : uint8_t {
    os_acpi_RESOURCE_COMPAT_PERF_GOOD,
    os_acpi_RESOURCE_COMPAT_PERF_ACCEPTABLE,
    os_acpi_RESOURCE_COMPAT_PERF_SUB_OPTIMAL,
};

struct os_acpi_resource_start_dependant {
    uint8_t length_kind;

    enum os_acpi_resource_compat_perf compat : 2;
    enum os_acpi_resource_compat_perf perf : 2;
};

struct os_acpi_resource_register_info {
    uint8_t address_space_id;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
};

enum os_acpi_resource_io_restriction : uint8_t {
    os_acpi_RESOURCE_IO_RESTRICTION_NONE,
    os_acpi_RESOURCE_IO_RESTRICTION_INPUT,
    os_acpi_RESOURCE_IO_RESTRICTION_OUTPUT,
    os_acpi_RESOURCE_IO_RESTRICTION_NONE_PRESERVED,
};

struct os_acpi_io_connection_flags {
    enum os_acpi_resource_io_restriction io_restriction : 2;
    bool is_shared : 1;
};

struct os_acpi_resource_gpio_info {
    uint8_t revision_id;
    uint8_t type;

    enum os_acpi_resource_direction direction : 1;

    union {
        struct os_acpi_irq_connection_info irq;
        struct os_acpi_io_connection_flags io;

        uint16_t type_specific;
    };

    uint8_t pull_configuration;
    uint16_t drive_strength;
    uint16_t debounce_timeout;
    uint16_t vendor_data_length;
    uint16_t pin_table_length;

    struct os_acpi_resource_source source;

    uint16_t *pin_table;
    uint8_t *vendor_data;
};

enum acpi_driver_resources_flags : uint16_t {
    ACPI_DRIVER_RESOURCES_IO = 1 << 0,
    ACPI_DRIVER_RESOURCES_MEM = 1 << 1,
    ACPI_DRIVER_RESOURCES_IRQ = 1 << 2,
    ACPI_DRIVER_RESOURCES_DMA = 1 << 3,
    ACPI_DRIVER_RESOURCES_ADDR = 1 << 4,
    ACPI_DRIVER_RESOURCES_DEP = 1 << 5,
    ACPI_DRIVER_RESOURCES_VENDOR = 1 << 6,
    ACPI_DRIVER_RESOURCES_REG = 1 << 7,
    ACPI_DRIVER_RESOURCES_GPIO = 1 << 8,

    ACPI_DRIVER_RESOURCES_ALL =
        ACPI_DRIVER_RESOURCES_IO |
        ACPI_DRIVER_RESOURCES_MEM |
        ACPI_DRIVER_RESOURCES_IRQ |
        ACPI_DRIVER_RESOURCES_DMA |
        ACPI_DRIVER_RESOURCES_ADDR |
        ACPI_DRIVER_RESOURCES_DEP |
        ACPI_DRIVER_RESOURCES_VENDOR |
        ACPI_DRIVER_RESOURCES_REG |
        ACPI_DRIVER_RESOURCES_GPIO,
};

struct os_acpi_device_resources {
    struct array irq_list;
    struct array dma_list;
    struct array fixed_dma_list;
    struct array io_list;
    struct array fixed_io_list;

    struct array addr16;
    struct array addr32;
    struct array addr64;
    struct array addr64_ext;

    struct array memory_list;
    struct array fixed_memory_list;
    struct array dep_list;
    struct array vendor_list;
    struct array reg_list;
    struct array gpio_list;
};

#define OS_ACPI_DEVICE_RESOURCES_INIT() \
    ((struct os_acpi_device_resources){ \
        .irq_list = ARRAY_INIT(sizeof(struct os_acpi_irq_info)), \
        .dma_list = ARRAY_INIT(sizeof(struct os_acpi_dma_info)), \
        .fixed_dma_list = ARRAY_INIT(sizeof(struct os_acpi_fixed_dma_info)), \
        .io_list = ARRAY_INIT(sizeof(struct os_acpi_io_info)), \
        .fixed_io_list = ARRAY_INIT(sizeof(struct os_acpi_fixed_io_info)), \
        .addr16 = ARRAY_INIT(sizeof(struct os_acpi_resource_address_u16)), \
        .addr32 = ARRAY_INIT(sizeof(struct os_acpi_resource_address_u32)), \
        .addr64 = ARRAY_INIT(sizeof(struct os_acpi_resource_address_u64)), \
        .addr64_ext = \
            ARRAY_INIT(sizeof(struct os_acpi_resource_address_u64_extended)), \
        .memory_list = ARRAY_INIT(sizeof(uint64_t)), \
        .fixed_memory_list = \
            ARRAY_INIT(sizeof(struct os_acpi_resource_fixed_memory_info)), \
        .dep_list = \
            ARRAY_INIT(sizeof(struct os_acpi_resource_start_dependant)), \
        .vendor_list = ARRAY_INIT(sizeof(struct string_view)), \
        .reg_list = ARRAY_INIT(sizeof(struct os_acpi_resource_register_info)), \
    })

bool
os_acpi_device_resources_collect(struct os_acpi_device_resources *resources,
                                 struct simple_alloc *alloc,
                                 uacpi_namespace_node *node,
                                 uint16_t flags);

void
os_acpi_device_resources_destroy(struct os_acpi_device_resources *resources);
