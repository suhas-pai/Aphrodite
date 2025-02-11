/*
 * kernel/src/acpi/resources.c
 * © suhas pai
 */

#include <lib/memory.h>
#include <lib/util.h>

#include <uacpi/namespace.h>
#include <uacpi/resources.h>

#include "acpi/resources.h"
#include "dev/printk.h"

bool
os_acpi_device_resources_collect(struct os_acpi_device_resources *const dev_res,
                                 struct simple_alloc *alloc,
                                 uacpi_namespace_node *const node,
                                 const uint16_t flags)
{
    if (flags == 0) {
        return true;
    }

    uacpi_resources *resources = nullptr;
    const uacpi_status ret = uacpi_get_current_resources(node, &resources);

    if (uacpi_unlikely_error(ret)) {
        const char *const path =
            uacpi_namespace_node_generate_absolute_path(node);

        printk(LOGLEVEL_WARN,
               "Unable to retrieve node %s's resources: %s\n",
               path,
               uacpi_status_to_string(ret));

        uacpi_free_absolute_path(path);
        return false;
    }

    uacpi_resource *iter = resources->entries;
    uint32_t length = resources->length;

    for (uint32_t offset = 0, index = 0;
         offset <= length;
         offset += iter->length,
         iter = reg_to_ptr(uacpi_resource, iter, iter->length),
         index++)
    {
        switch ((enum uacpi_resource_type)iter->type) {
            case UACPI_RESOURCE_TYPE_IRQ: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_IRQ) == 0) {
                    continue;
                }

                const struct uacpi_resource_irq *const irq = &iter->irq;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*irq)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: irq resource out of bounds\n");

                    return false;
                }

                struct os_acpi_irq_info info = {
                    .polarity =
                        irq->polarity == UACPI_POLARITY_ACTIVE_HIGH
                            ? IRQ_POLARITY_HIGH
                            : IRQ_POLARITY_LOW,
                    .trigger_mode = (enum irq_trigger_mode)irq->triggering,
                    .is_shared = irq->sharing == UACPI_SHARED,
                    .wake_capable = irq->wake_capability == UACPI_WAKE_CAPABLE,
                    .irq_count = irq->num_irqs,
                    .irq_list = nullptr,
                };

                if (info.irq_count == 0) {
                    if (!array_add(&dev_res->irq_list, &info)) {
                        uacpi_free_resources(resources);
                        printk(LOGLEVEL_WARN,
                               "acpi/resources: failed to add irq-info\n");

                        return false;
                    }

                    continue;
                }

                info.irq_list =
                    simple_alloc(alloc,
                                 sizeof(*info.irq_list) * iter->irq.num_irqs);

                if (info.irq_list == nullptr) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to allocate memory for "
                           "irq-info\n");

                    return false;
                }

                for_upto_limit(iter->irq.num_irqs, i) {
                    info.irq_list[i] = irq->irqs[i];
                }

                if (!array_add(&dev_res->irq_list, &info)) {
                    simple_free(alloc, info.irq_list);
                    uacpi_free_resources(resources);

                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add irq-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_IRQ) == 0) {
                    continue;
                }

                const auto irq = &iter->extended_irq;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*irq)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: extended-irq resource out of "
                           "bounds\n");

                    return false;
                }

                struct os_acpi_irq_info info = {
                    .polarity =
                        irq->polarity == UACPI_POLARITY_ACTIVE_HIGH
                            ? IRQ_POLARITY_HIGH
                            : IRQ_POLARITY_LOW,
                    .trigger_mode = (enum irq_trigger_mode)irq->triggering,
                    .is_shared = irq->sharing == UACPI_SHARED,
                    .wake_capable = irq->wake_capability == UACPI_WAKE_CAPABLE,
                    .irq_count = irq->num_irqs,
                    .irq_list = nullptr,
                };

                if (info.irq_count == 0) {
                    if (!array_add(&dev_res->irq_list, &info)) {
                        uacpi_free_resources(resources);
                        printk(LOGLEVEL_WARN,
                               "acpi/resources: failed to add irq-info\n");

                        return false;
                    }

                    continue;
                }

                info.irq_list =
                    simple_alloc(alloc,
                                 sizeof(info.irq_list) * iter->irq.num_irqs);

                if (info.irq_list == nullptr) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to allocate memory for "
                           "irq-info\n");

                    return false;
                }

                memcpy32(info.irq_list, irq->irqs, info.irq_count);
                if (!array_add(&dev_res->irq_list, &info)) {
                    simple_free(alloc, info.irq_list);
                    uacpi_free_resources(resources);

                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add irq-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_DMA: {
                if ((flags & UACPI_RESOURCE_TYPE_DMA) == 0) {
                    continue;
                }

                const auto dma = &iter->dma;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*dma)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: dma resource out of bounds\n");

                    return false;
                }

                struct os_acpi_dma_info info = {
                    .channel_count = dma->num_channels,
                    .channel_list = nullptr,
                };

                if (info.channel_count == 0) {
                    if (!array_add(&dev_res->dma_list, &info)) {
                        uacpi_free_resources(resources);
                        printk(LOGLEVEL_WARN,
                               "acpi/resources: failed to add dma-info\n");

                        return false;
                    }

                    continue;
                }

                info.channel_list =
                    simple_alloc(
                        alloc,
                        sizeof(*info.channel_list) * info.channel_count);

                if (info.channel_list == nullptr) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to allocate memory for "
                           "dma-info\n");

                    return false;
                }

                memcpy(info.channel_list, dma->channels, info.channel_count);
                continue;
            }
            case UACPI_RESOURCE_TYPE_FIXED_DMA: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_DMA) == 0) {
                    continue;
                }

                const auto dma = &iter->fixed_dma;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*dma)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: fixed-dma resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_fixed_dma_info info = {
                    .request_line = dma->request_line,
                    .channel = dma->channel,
                    .transfer_width = dma->transfer_width,
                };

                if (!array_add(&dev_res->fixed_dma_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add fixed-dma-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_IO: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_IO) == 0) {
                    continue;
                }

                const auto io = &iter->io;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*io)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: io resource out of bounds\n");

                    return false;
                }

                const struct os_acpi_io_info info = {
                    .decode_kind = (enum os_acpi_io_decode_kind)io->decode_type,
                    .minimum = io->minimum,
                    .maximum = io->maximum,
                    .alignment = io->alignment,
                    .length = io->length,
                };

                if (!array_add(&dev_res->io_list, &info)) {
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add io-info\n");
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_FIXED_IO: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_IO) == 0) {
                    continue;
                }

                const auto io = &iter->fixed_io;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*io)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: fixed-io resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_fixed_io_info info = {
                    .address = io->address,
                    .length = io->length,
                };

                if (!array_add(&dev_res->fixed_io_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add fixed-io-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_ADDRESS16: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_ADDR) == 0) {
                    continue;
                }

                const auto addr = &iter->address16;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*addr)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: address16 resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_resource_address_u16 info = {
                    .attribute = {
                        .mem_attr = {
                            .cache_kind = addr->common.attribute.memory.caching,
                            .range_kind =
                                addr->common.attribute.memory.range_type,
                            .translation_kind =
                                addr->common.attribute.memory.translation,
                            .writable =
                                addr->common.attribute.memory.write_status ==
                                UACPI_WRITABLE,
                        },
                        .io_attr = {
                            .range_kind =
                                addr->common.attribute.io.range_type,
                            .mem_kind = addr->common.attribute.io.translation,
                            .translation_kind =
                                addr->common.attribute.io.translation_type,
                        },
                        .type_specific = addr->common.attribute.type_specific,
                    },
                    .type = addr->common.type,
                    .direction = addr->common.direction,
                    .decode_kind = addr->common.decode_type,
                    .fixed_min_address = addr->common.fixed_min_address,
                    .fixed_max_address = addr->common.fixed_max_address,
                    .minimum = addr->minimum,
                    .maximum = addr->maximum,
                    .translation_offset = addr->translation_offset,
                    .granularity = addr->granularity,
                    .source = {
                        .index = addr->source.index,
                        .index_valid = addr->source.index_present,
                        .length = addr->source.length,
                        .string = addr->source.string,
                    },
                };

                if (!array_add(&dev_res->addr16, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add address16-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_ADDRESS32: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_ADDR) == 0) {
                    continue;
                }

                const auto addr = &iter->address32;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*addr)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: address32 resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_resource_address_u32 info = {
                    .attribute = {
                        .mem_attr = {
                            .cache_kind = addr->common.attribute.memory.caching,
                            .range_kind =
                                addr->common.attribute.memory.range_type,
                            .translation_kind =
                                addr->common.attribute.memory.translation,
                            .writable =
                                addr->common.attribute.memory.write_status ==
                                UACPI_WRITABLE,
                        },
                        .io_attr = {
                            .range_kind =
                                addr->common.attribute.io.range_type,
                            .mem_kind = addr->common.attribute.io.translation,
                            .translation_kind =
                                addr->common.attribute.io.translation_type,
                        },
                        .type_specific = addr->common.attribute.type_specific,
                    },
                    .type = addr->common.type,
                    .direction = addr->common.direction,
                    .decode_kind = addr->common.decode_type,
                    .fixed_min_address = addr->common.fixed_min_address,
                    .fixed_max_address = addr->common.fixed_max_address,
                    .minimum = addr->minimum,
                    .maximum = addr->maximum,
                    .translation_offset = addr->translation_offset,
                    .granularity = addr->granularity,
                    .source = {
                        .index = addr->source.index,
                        .index_valid = addr->source.index_present,
                        .length = addr->source.length,
                        .string = addr->source.string,
                    },
                };

                if (!array_add(&dev_res->addr32, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add address16-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_ADDRESS64: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_ADDR) == 0) {
                    continue;
                }

                const auto addr = &iter->address64;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*addr)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: address64 resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_resource_address_u64 info = {
                    .attribute = {
                        .mem_attr = {
                            .cache_kind = addr->common.attribute.memory.caching,
                            .range_kind =
                                addr->common.attribute.memory.range_type,
                            .translation_kind =
                                addr->common.attribute.memory.translation,
                            .writable =
                                addr->common.attribute.memory.write_status ==
                                UACPI_WRITABLE,
                        },
                        .io_attr = {
                            .range_kind =
                                addr->common.attribute.io.range_type,
                            .mem_kind = addr->common.attribute.io.translation,
                            .translation_kind =
                                addr->common.attribute.io.translation_type,
                        },
                        .type_specific = addr->common.attribute.type_specific,
                    },
                    .type = addr->common.type,
                    .direction = addr->common.direction,
                    .decode_kind = addr->common.decode_type,
                    .fixed_min_address = addr->common.fixed_min_address,
                    .fixed_max_address = addr->common.fixed_max_address,
                    .minimum = addr->minimum,
                    .maximum = addr->maximum,
                    .translation_offset = addr->translation_offset,
                    .granularity = addr->granularity,
                    .source = {
                        .index = addr->source.index,
                        .index_valid = addr->source.index_present,
                        .length = addr->source.length,
                        .string = addr->source.string,
                    },
                };

                if (!array_add(&dev_res->addr64, &info)) {
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add address16-info\n");
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_ADDR) == 0) {
                    continue;
                }

                const auto addr = &iter->address64_extended;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*addr)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: address64-extended resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_resource_address_u64_extended info = {
                    .attribute = {
                        .mem_attr = {
                            .cache_kind = addr->common.attribute.memory.caching,
                            .range_kind =
                                addr->common.attribute.memory.range_type,
                            .translation_kind =
                                addr->common.attribute.memory.translation,
                            .writable =
                                addr->common.attribute.memory.write_status ==
                                UACPI_WRITABLE,
                        },
                        .io_attr = {
                            .range_kind = addr->common.attribute.io.range_type,
                            .mem_kind = addr->common.attribute.io.translation,
                            .translation_kind =
                                addr->common.attribute.io.translation_type,
                        },
                        .type_specific = addr->common.attribute.type_specific,
                    },
                    .type = addr->common.type,
                    .direction = addr->common.direction,
                    .decode_kind = addr->common.decode_type,
                    .fixed_min_address = addr->common.fixed_min_address,
                    .fixed_max_address = addr->common.fixed_max_address,
                    .minimum = addr->minimum,
                    .maximum = addr->maximum,
                    .translation_offset = addr->translation_offset,
                    .granularity = addr->granularity,
                    .address_length = addr->address_length,
                    .attributes = addr->attributes
                };

                if (!array_add(&dev_res->addr64_ext, &info)) {
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add address16-info\n");
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_MEMORY24: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_MEM) == 0) {
                    continue;
                }

                const auto mem = &iter->memory24;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*mem)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: memory24 resource out of bounds\n");

                    return false;
                }

                const struct os_acpi_resource_memory_info info = {
                    .minimum = mem->minimum,
                    .maximum = mem->maximum,
                    .alignment = mem->alignment,
                    .length = mem->length,
                    .writable = mem->write_status == UACPI_WRITABLE,
                };

                if (!array_add(&dev_res->memory_list, &info)) {
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add memory-info\n");
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_MEMORY32: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_MEM) == 0) {
                    continue;
                }

                const auto mem = &iter->memory32;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*mem)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: memory32 resource out of bounds\n");

                    return false;
                }

                const struct os_acpi_resource_memory_info info = {
                    .minimum = mem->minimum,
                    .maximum = mem->maximum,
                    .alignment = mem->alignment,
                    .length = mem->length,
                    .writable = mem->write_status == UACPI_WRITABLE,
                };

                if (!array_add(&dev_res->memory_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add memory-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_FIXED_MEMORY32: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_MEM) == 0) {
                    continue;
                }

                const auto mem = &iter->fixed_memory32;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*mem)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: fixed-memory32 resource out of "
                           "bounds\n");

                    return false;
                }

                const struct os_acpi_resource_fixed_memory_info info = {
                    .address = mem->address,
                    .length = mem->length,
                    .writable = mem->write_status == UACPI_WRITABLE,
                };

                if (!array_add(&dev_res->memory_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add memory-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_START_DEPENDENT: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_DEP) == 0) {
                    continue;
                }

                const auto dep = &iter->start_dependent;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*dep)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: dependent resource out of bounds\n");

                    return false;
                }

                const struct os_acpi_resource_start_dependant info = {
                    .length_kind = dep->length_kind,
                    .compat = dep->compatibility,
                    .perf = dep->performance,
                };

                if (!array_add(&dev_res->dep_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add dependent-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_VENDOR_SMALL:
            case UACPI_RESOURCE_TYPE_VENDOR_LARGE: {
                if ((flags &
                        __OS_ACPI_DRIVER_RESOURCES_VENDOR) == 0)
                {
                    continue;
                }

                const auto vendor = &iter->vendor;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*vendor)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: vendor resource out of bounds\n");

                    return false;
                }

                const struct string_view vendor_sv =
                    sv_create_nocheck((const char *)vendor->data,
                                      vendor->length);

                if (!array_add(&dev_res->vendor_list, &vendor_sv)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add vendor-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_GENERIC_REGISTER: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_REG) == 0) {
                    continue;
                }

                const auto reg = &iter->generic_register;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*reg)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: vendor resource out of bounds\n");

                    return false;
                }

                const struct os_acpi_resource_register_info info = {
                    .address_space_id = reg->address_space_id,
                    .bit_width = reg->bit_width,
                    .bit_offset = reg->bit_offset,
                    .access_size = reg->access_size,
                    .address = reg->address,
                };

                if (!array_add(&dev_res->reg_list, &info)) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add register-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_GPIO_CONNECTION: {
                if ((flags & __OS_ACPI_DRIVER_RESOURCES_GPIO) == 0) {
                    continue;
                }

                const auto gpio = &iter->gpio_connection;
                if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(*gpio)),
                                           length))
                {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: vendor resource out of bounds\n");

                    return false;
                }

                struct os_acpi_resource_gpio_info info = {
                    .revision_id = gpio->revision_id,
                    .type = gpio->type,
                    .direction = gpio->direction,
                    .pull_configuration = gpio->pull_configuration,
                    .drive_strength = gpio->drive_strength,
                    .debounce_timeout = gpio->debounce_timeout,
                    .vendor_data_length = gpio->vendor_data_length,
                    .pin_table_length = gpio->pin_table_length,
                    .source = {
                        .index = gpio->source.index,
                        .index_valid = gpio->source.index_present,
                        .length = gpio->source.length,
                        .string = gpio->source.string,
                    }
                };

                info.pin_table = simple_alloc(alloc, info.pin_table_length);
                if (info.pin_table == nullptr) {
                    uacpi_free_resources(resources);
                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add pin-table\n");

                    return false;
                }

                info.vendor_data =
                    simple_alloc(alloc, info.vendor_data_length);

                if (info.vendor_data == nullptr) {
                    simple_free(alloc, info.pin_table);
                    uacpi_free_resources(resources);

                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add vendor-data\n");

                    return false;
                }

                memcpy(info.pin_table, gpio->pin_table,
                       info.pin_table_length);
                memcpy(info.vendor_data, gpio->vendor_data,
                       info.vendor_data_length);

                if (!array_add(&dev_res->gpio_list, &info)) {
                    simple_free(alloc, info.pin_table);
                    simple_free(alloc, info.vendor_data);

                    printk(LOGLEVEL_WARN,
                           "acpi/resources: failed to add gpio-info\n");

                    return false;
                }

                continue;
            }
            case UACPI_RESOURCE_TYPE_END_DEPENDENT:
            case UACPI_RESOURCE_TYPE_SERIAL_I2C_CONNECTION:
            case UACPI_RESOURCE_TYPE_SERIAL_SPI_CONNECTION:
            case UACPI_RESOURCE_TYPE_SERIAL_UART_CONNECTION:
            case UACPI_RESOURCE_TYPE_SERIAL_CSI2_CONNECTION:
            case UACPI_RESOURCE_TYPE_PIN_FUNCTION:
            case UACPI_RESOURCE_TYPE_PIN_CONFIGURATION:
            case UACPI_RESOURCE_TYPE_PIN_GROUP:
            case UACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION:
            case UACPI_RESOURCE_TYPE_PIN_GROUP_CONFIGURATION:
            case UACPI_RESOURCE_TYPE_CLOCK_INPUT:
                // TODO:
                continue;
            case UACPI_RESOURCE_TYPE_END_TAG:
                uacpi_free_resources(resources);
                return true;
        }

        verify_not_reached();
    }

    verify_not_reached();
}

void
os_acpi_device_resources_destroy(
    struct os_acpi_device_resources *const resources)
{
    array_destroy(&resources->irq_list);
    array_destroy(&resources->dma_list);
    array_destroy(&resources->fixed_dma_list);
    array_destroy(&resources->io_list);
    array_destroy(&resources->fixed_io_list);

    array_destroy(&resources->addr16);
    array_destroy(&resources->addr32);
    array_destroy(&resources->addr64);
    array_destroy(&resources->addr64_ext);

    array_destroy(&resources->memory_list);
    array_destroy(&resources->fixed_memory_list);
    array_destroy(&resources->dep_list);
    array_destroy(&resources->vendor_list);
    array_destroy(&resources->reg_list);
    array_destroy(&resources->gpio_list);
}
