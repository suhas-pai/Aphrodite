/*
 * kernel/src/arch/x86_64/apic/ioapic.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "dev/printk.h"
#include "lib/align.h"
#include "sys/mmio.h"

#include "ioapic.h"

static struct array g_ioapic_list =
    ARRAY_INIT(sizeof(struct ioapic_info));

__optimize(3) uint64_t
create_ioapic_redirect_request(
    const uint8_t vector,
    const enum ioapic_redirect_req_delivery_mode delivery_mode,
    const enum ioapic_redirect_req_dest_mode dest_mode,
    const uint32_t flags,
    const bool masked,
    const uint8_t lapic_id)
{
    const bool is_active_low = flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW;
    const bool is_level_triggered = flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER;

    const uint64_t result =
        vector |
        delivery_mode << 8 |
        dest_mode << 11 |
        (uint32_t)is_active_low << 13 |
        (uint32_t)is_level_triggered << 15 |
        (uint32_t)masked << 16 |
        (uint64_t)lapic_id << 56;

    return result;
}

__optimize(3)
static struct ioapic_info *ioapic_info_for_gsi(const uint32_t gsi) {
    array_foreach(&g_ioapic_list, struct ioapic_info, item) {
        const uint32_t gsi_base = item->gsi_base;
        if (gsi > gsi_base && gsi < gsi_base + item->max_redirect_count) {
            return item;
        }
    }

    return NULL;
}

static void
redirect_irq(const uint8_t lapic_id,
             const uint8_t irq,
             const uint8_t vector,
             const uint16_t flags,
             const bool masked)
{
    const struct ioapic_info *const ioapic = ioapic_info_for_gsi(/*gsi=*/irq);
    assert_msg(ioapic != NULL,
               "ioapic: failed to find i/o apic for requested IRQ: %" PRIu8,
               irq);

    const uint8_t redirect_table_index = irq - ioapic->gsi_base;
    const uint64_t req_value =
        create_ioapic_redirect_request(vector,
                                       IOAPIC_REDIRECT_REQ_DELIVERY_MODE_FIXED,
                                       IOAPIC_REDIRECT_REQ_DEST_MODE_PHYSICAL,
                                       flags,
                                       masked,
                                       lapic_id);

    const uint32_t reg =
        ioapic_redirect_table_get_reg_for_n(redirect_table_index);

    // For selector=reg, write the lower-32 bits, for selector=reg + 1, write
    // the upper-32 bits.

    ioapic_write(ioapic, reg, req_value);
    ioapic_write(ioapic, reg + 1, req_value >> 32);
}

void
ioapic_add(const uint8_t apic_id, const uint32_t base, const uint32_t gsib) {
    if (!has_align(base, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "ioapic: io-apic with apic-id %" PRIu8 ", gsib %" PRIu32 ", "
               "base %p is not aligned on a page boundary. ignoring\n",
               apic_id,
               gsib,
               (void *)(uint64_t)base);
        return;
    }

    struct ioapic_info info = {
        .arbid = apic_id,
        .gsi_base = gsib,
        .regs_mmio =
            vmap_mmio(RANGE_INIT(base, PAGE_SIZE),
                      PROT_READ | PROT_WRITE,
                      /*flags=*/0)
    };

    assert_msg(info.regs_mmio != NULL, "ioapic: failed to map ioapic regs");
    info.regs = info.regs_mmio->base;

    const uint32_t id_reg = ioapic_read(&info, IOAPIC_REG_ID);
    assert_msg(info.arbid == ioapic_id_reg_get_arbid(id_reg),
               "io-apic ID in MADT doesn't match ID in MMIO");

    const uint32_t ioapic_version_reg = ioapic_read(&info, IOAPIC_REG_VERSION);

    info.version = ioapic_version_reg_get_version(ioapic_version_reg);
    info.max_redirect_count =
        ioapic_version_reg_get_max_redirect_count(ioapic_version_reg);

    printk(LOGLEVEL_INFO,
           "ioapic: added ioapic with version: %" PRIu8 " and max "
           "redirect-count: %" PRIu8 ", mmio: " RANGE_FMT "\n",
           info.version,
           info.max_redirect_count,
           RANGE_FMT_ARGS(mmio_region_get_range(info.regs_mmio)));

    assert_msg(array_append(&g_ioapic_list, &info),
               "ioapic: failed to add io-apic base to array");
}

__optimize(3) uint32_t
ioapic_read(const struct ioapic_info *const ioapic, const enum ioapic_reg reg) {
    volatile struct ioapic_registers *const regs = ioapic->regs;

    mmio_write(&regs->selector, (uint32_t)reg);
    return mmio_read(&regs->data);
}

__optimize(3) void
ioapic_write(const struct ioapic_info *const ioapic,
             const enum ioapic_reg reg,
             const uint32_t value)
{
    volatile struct ioapic_registers *const regs = ioapic->regs;

    mmio_write(&regs->selector, (uint32_t)reg);
    mmio_write(&regs->data, value);
}

void
ioapic_redirect_irq(const uint8_t lapic_id,
                    const uint8_t irq,
                    const uint8_t vector,
                    const bool masked)
{
    // First check if the IOAPIC already directs this IRQ to the requested
    // vector. If so, we don't need to do anything.

    array_foreach(&get_acpi_info()->iso_list, struct apic_iso_info, item) {
        if (item->irq_src != irq) {
            continue;
        }

        // Don't fill up the redirection-table if the iso already directs
        // the irq to the requested irq.

        const uint8_t gsi = item->gsi;
        if (gsi == vector) {
            return;
        }

        // Create a redirection-request for the requested vector with an iso
        // (Interrupt Source Override) entry.

        redirect_irq(lapic_id, gsi, vector, item->flags, masked);
        return;
    }

    // With no ISO found, we simply create a redirection-request for the irq we
    // were given.

    redirect_irq(lapic_id, irq, vector, /*flags=*/0, masked);
}