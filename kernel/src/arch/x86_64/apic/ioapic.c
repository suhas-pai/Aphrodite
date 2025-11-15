/*
 * kernel/src/arch/x86_64/apic/ioapic.c
 * © suhas pai
 */

#include <lib/align.h>

#include "acpi/api.h"
#include "apic/ioapic.h"

#include "dev/printk.h"
#include "sys/mmio.h"

static struct array g_ioapic_list = ARRAY_INIT(sizeof(struct ioapic_info));

enum ioapic_redir_reg_shifts : uint8_t {
    IOAPIC_REDIRECT_REG_VECTOR_SHIFT = 0,
    IOAPIC_REDIRECT_REG_DELIVERY_MODE_SHIFT = 8,
    IOAPIC_REDIRECT_REG_DEST_MODE_SHIFT = 11,
    IOAPIC_REDIRECT_REG_POLARITY_SHIFT = 13,
    IOAPIC_REDIRECT_REG_TRIGGER_MODE_SHIFT = 15,
    IOAPIC_REDIRECT_REG_MASKED_SHIFT = 16,
    IOAPIC_REDIRECT_REG_LAPIC_ID_SHIFT = 56
};

__debug_optimize(3) uint64_t
ioapic_redirect_request_create(
    const uint8_t vector,
    const enum ioapic_redirect_req_delivery_mode delivery_mode,
    const enum ioapic_redirect_req_dest_mode dest_mode,
    const uint32_t flags,
    const bool masked,
    const uint8_t lapic_id)
{
    const bool is_active_low = flags & __OS_ACPI_MADT_ENTRY_ISO_ACTIVE_LOW;
    const bool is_level_triggered =
        flags & __OS_ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER;

    const uint64_t result =
        vector
      | (uint64_t)delivery_mode << IOAPIC_REDIRECT_REG_DELIVERY_MODE_SHIFT
      | (uint64_t)dest_mode << IOAPIC_REDIRECT_REG_DEST_MODE_SHIFT
      | (uint32_t)is_active_low << IOAPIC_REDIRECT_REG_POLARITY_SHIFT
      | (uint32_t)is_level_triggered << IOAPIC_REDIRECT_REG_TRIGGER_MODE_SHIFT
      | (uint32_t)masked << IOAPIC_REDIRECT_REG_MASKED_SHIFT
      | (uint64_t)lapic_id << IOAPIC_REDIRECT_REG_LAPIC_ID_SHIFT;

    return result;
}

__debug_optimize(3)
static const struct ioapic_info *ioapic_info_for_gsi(const uint32_t gsi) {
    array_foreach(&g_ioapic_list, const struct ioapic_info, item) {
        const uint32_t gsi_base = item->gsi_base;
        if (gsi > gsi_base && gsi < gsi_base + item->max_redirect_count) {
            return item;
        }
    }

    return nullptr;
}

static void
redirect_irq(const uint8_t lapic_id,
             const uint8_t irq,
             const uint8_t vector,
             const uint16_t flags,
             const bool masked)
{
    const struct ioapic_info *const ioapic = ioapic_info_for_gsi(/*gsi=*/irq);
    assert_msg(ioapic != nullptr,
               "ioapic: failed to find i/o apic for requested IRQ: %" PRIu8,
               irq);

    const uint8_t redirect_table_index = irq - ioapic->gsi_base;
    const uint64_t req_value =
        ioapic_redirect_request_create(vector,
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

static void toggle_irq_mask(const uint8_t irq, const bool masked) {
    const struct ioapic_info *const ioapic = ioapic_info_for_gsi(/*gsi=*/irq);
    assert_msg(ioapic != nullptr,
               "ioapic: failed to find i/o apic for requested IRQ: %" PRIu8,
               irq);

    const uint8_t redirect_table_index = irq - ioapic->gsi_base;
    const uint32_t reg =
        ioapic_redirect_table_get_reg_for_n(redirect_table_index);

    uint64_t reg_value =
        ioapic_read(ioapic, reg) | (uint64_t)ioapic_read(ioapic, reg + 1) << 32;

    if (masked) {
        reg_value |= 1 << IOAPIC_REDIRECT_REG_MASKED_SHIFT;
    } else {
        reg_value = rm_mask(reg_value, IOAPIC_REDIRECT_REG_MASKED_SHIFT);
    }

    ioapic_write(ioapic, reg, reg_value);
    ioapic_write(ioapic, reg + 1, reg_value >> 32);
}

void
ioapic_add(const uint8_t apic_id, const uint32_t base, const uint32_t gsib) {
    if (!is_page_aligned(base)) {
        printk(LOGLEVEL_WARN,
               "ioapic: io-apic with apic-id %" PRIu8 ", gsib %" PRIu32 ", "
               "base %p is not aligned on a page boundary. ignoring\n",
               apic_id,
               gsib,
               cast_to_ptr(void *, base));
        return;
    }

    struct range range = RANGE_EMPTY();
    if (!range_create_and_verify(base, PAGE_SIZE, &range)) {
        printk(LOGLEVEL_WARN,
               "ioapic: io-apic with apic-id %" PRIu8 ", gsib %" PRIu32 ", "
               "base %p overflows. ignoring\n",
               apic_id,
               gsib,
               cast_to_ptr(void *, base));
        return;
    }

    struct ioapic_info info = {
        .arbid = apic_id,
        .gsi_base = gsib,
        .regs_mmio = vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0)
    };

    if (info.regs_mmio == nullptr) {
        printk(LOGLEVEL_WARN, "ioapic: failed to map ioapic regs");
        return;
    }

    info.regs = info.regs_mmio->base;

    const uint32_t id_reg = ioapic_read(&info, IOAPIC_REG_ID);
    assert_msg(info.arbid == ioapic_id_reg_get_arbid(id_reg),
               "io-apic ID in MADT doesn't match ID in MMIO");

    const uint32_t ioapic_version_reg = ioapic_read(&info, IOAPIC_REG_VERSION);

    info.version = ioapic_version_reg_get_version(ioapic_version_reg);
    info.max_redirect_count =
        ioapic_version_reg_get_max_redirect_count(ioapic_version_reg);

    const struct range mmio_range = mmio_region_get_range(info.regs_mmio);
    printk(LOGLEVEL_INFO,
           "ioapic: added ioapic\n"
           "\t\t" "version: %" PRIu8 "\n"
           "\t\t" "max redirect-count: %" PRIu8 "\n"
           "\t\t" "mmio: " RANGE_FMT "\n",
           info.version,
           info.max_redirect_count,
           RANGE_FMT_ARGS(mmio_range));

    assert_msg(array_add(&g_ioapic_list, &info),
               "ioapic: failed to add io-apic base to array");
}

__debug_optimize(3) uint32_t
ioapic_read(const struct ioapic_info *const ioapic, const enum ioapic_reg reg) {
    volatile struct ioapic_registers *const regs = ioapic->regs;

    mmio_write(&regs->selector, (uint32_t)reg);
    return mmio_read(&regs->data);
}

__debug_optimize(3) void
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
    array_foreach(&get_acpi_info()->iso_list, const struct apic_iso_info, iso) {
        if (iso->irq_src != irq) {
            continue;
        }

        // Don't fill up the redirection-table if the iso already directs the
        // irq to the requested irq.

        const uint8_t gsi = iso->gsi;
        if (gsi == vector) {
            return;
        }

        // Create a redirection-request for the requested vector with an iso
        // (Interrupt Source Override) entry.

        redirect_irq(lapic_id, gsi, vector, iso->flags, masked);
        return;
    }

    // With no ISO found, we simply create a redirection-request for the irq we
    // were given.

    redirect_irq(lapic_id, irq, vector, /*flags=*/0, masked);
}

void ioapic_toggle_irq_mask(const uint8_t irq, const bool masked) {
    array_foreach(&get_acpi_info()->iso_list, const struct apic_iso_info, iso) {
        if (iso->irq_src == irq) {
            toggle_irq_mask(iso->gsi, masked);
            return;
        }
    }

    toggle_irq_mask(irq, masked);
}
