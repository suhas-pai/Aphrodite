/*
 * kernel/src/arch/x86_64/dev/ide/init.c
 * © suhas pai
 */

#include "dev/ata/defines.h"
#include "dev/pci/structs.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/size.h"
#include "lib/util.h"

#include "sys/pio.h"

#define PCI_IDE_BAR_INDEX 4
#define IDE_ATA 0x00
#define IDE_ATAPI 0x01

#define ATA_MASTER_DRIVE 0x00
#define ATA_SLAVE_DRIVE 0x01

#define ATA_PRIMARY 0x00
#define ATA_SECONDARY 0x01

struct ide_channel {
   uint16_t base;
   uint16_t ctrl;
   uint16_t bmide;
   uint8_t nIEN;
};

struct ide_device {
   uint8_t reserved;
   uint8_t channel;
   uint8_t drive;
   uint16_t type;
   uint16_t signature;
   uint16_t capabilities;
   uint32_t command_sets;
   uint32_t size;
   uint8_t model[41];
};

static struct ide_device g_devices_list[4] = {0};
static struct ide_channel g_channel_list[2] = {0};

void ide_write(const uint8_t channel, const uint8_t reg, const uint8_t data) {
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channel_list[channel].nIEN);
    }

    if (reg < 0x08) {
        pio_write8(g_channel_list[channel].base + reg - 0x00, data);
    } else if (reg < 0x0C) {
        pio_write8(g_channel_list[channel].base + reg - 0x06, data);
    } else if (reg < 0x0E) {
        pio_write8(g_channel_list[channel].ctrl + reg - 0x0A, data);
    } else if (reg < 0x16) {
        pio_write8(g_channel_list[channel].bmide + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, g_channel_list[channel].nIEN);
    }
}

uint8_t ide_read(const uint8_t channel, const uint8_t reg) {
    uint8_t result = 0;
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channel_list[channel].nIEN);
    }

    if (reg < 0x08) {
        result = pio_read8(g_channel_list[channel].base + reg - 0x00);
    } else if (reg < 0x0C) {
        result = pio_read8(g_channel_list[channel].base + reg - 0x06);
    } else if (reg < 0x0E) {
        result = pio_read8(g_channel_list[channel].ctrl + reg - 0x0A);
    } else if (reg < 0x16) {
        result = pio_read8(g_channel_list[channel].bmide + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, g_channel_list[channel].nIEN);
    }

    return result;
}

#define insl(port, buffer, count) \
    asm volatile ("cld; rep; insl" :: "D" (buffer), "d" (port), "c" (count))

#define insw(port, buffer, count) \
    asm volatile ("rep insw" :: "D"(buffer), "d"(port), "c"(count))

#define insb(port, buffer, count) \
    asm volatile ("rep insb" :: "D"(buffer), "d"(port), "c"(count))

#define outsw(port, buffer, count) \
    asm volatile ("rep outsw" :: "c"(count), "d"(port), "S"(edi))

void
ide_read_buffer(const uint8_t channel,
                const uint8_t reg,
                char *const buffer,
                const uint32_t quads)
{
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channel_list[channel].nIEN);
    }

    if (reg < 0x08) {
        insl(g_channel_list[channel].base  + reg - 0x00, buffer, quads);
    } else if (reg < 0x0C) {
        insl(g_channel_list[channel].base  + reg - 0x06, buffer, quads);
    } else if (reg < 0x0E) {
        insl(g_channel_list[channel].ctrl  + reg - 0x0A, buffer, quads);
    } else if (reg < 0x16) {
        insl(g_channel_list[channel].bmide + reg - 0x0E, buffer, quads);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, g_channel_list[channel].nIEN);
    }
}

enum ide_polling_result {
    IDE_POLLING_OK,
    IDE_POLLING_DEVICE_FAULT,
    IDE_POLLING_ATA_ERROR,
    IDE_POLLING_NOTHING_READ,
    IDE_POLLING_WRITE_PROTECTED
};

enum ide_polling_result
ide_polling(const uint8_t channel, const uint32_t advanced_check) {
    // (I) Delay 400 nanosecond for BSY to be set:
    for (uint8_t i = 0; i < 4; i++) {
        // Reading the Alternate Status port wastes 100ns; loop four times.
        ide_read(channel, ATA_REG_ALT_STATUS);
    }

    // (II) Wait for BSY to be cleared:
    while (ide_read(channel, ATA_REG_STATUS) & __ATA_STATUS_REG_BSY) {}
    if (!advanced_check) {
        return IDE_POLLING_OK;
    }

    // Read Status Register.
    const uint8_t state = ide_read(channel, ATA_REG_STATUS);

    // (III) Check For Errors:
    if (state & __ATA_STATUS_REG_ERR) {
        return IDE_POLLING_ATA_ERROR;
    }

    // (IV) Check If Device fault:
    if (state & __ATA_STATUS_REG_DF) {
        return IDE_POLLING_DEVICE_FAULT;
    }

    // (V) Check DRQ:
    if ((state & __ATA_STATUS_REG_DRQ) == 0) {
        return IDE_POLLING_NOTHING_READ;
    }

    return IDE_POLLING_OK; // No Error.
}

static char ide_buf[2048] = {0};

static void
handle_device(struct ide_device *const device,
              const uint32_t channel,
              const uint32_t drive)
{
    device->reserved = 0; // Assuming that no drive here.

    // (I) Select Drive:
    ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (drive << 4)); // Select Drive.
    // sleep(1); // Wait 1ms for drive select to work.

    // (II) Send ATA Identify Command:
    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    //sleep(1);

    // (III) Polling:
    if (ide_read(channel, ATA_REG_STATUS) == 0) {
        return; // If Status = 0, No Device.
    }

    uint8_t error = 0;
    while (true) {
        const uint8_t status = ide_read(channel, ATA_REG_STATUS);
        if (status & __ATA_STATUS_REG_ERR) {
            error = 1;
            break;
        }

        if ((status & __ATA_STATUS_REG_BSY) == 0 &&
            (status & __ATA_STATUS_REG_DRQ))
        {
            break;
        }
    }

    // (IV) Probe for ATAPI Devices:
    uint8_t type = IDE_ATA;
    if (error != 0) {
        const uint8_t cl = ide_read(channel, ATA_REG_LBA1);
        const uint8_t ch = ide_read(channel, ATA_REG_LBA2);

        if ((cl != 0x14 || ch != 0xEB) && (cl != 0x69 || ch != 0x96)) {
            // Unknown Type (may not be a device).
            return;
        }

        type = IDE_ATAPI;
        ide_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
        //sleep(1);
    }

    // (V) Read Identification Space of the Device:
    ide_read_buffer(channel, ATA_REG_DATA, ide_buf, /*quads=*/128);

    const struct ata_identify *const ide_ident =
        (const struct ata_identify *)ide_buf;

    // (VI) Read Device Parameters:
    device->reserved = 1;
    device->type = type;
    device->channel = channel;
    device->drive = drive;
    device->signature = ide_ident->device_type;
    device->capabilities = ide_ident->capabilities;
    device->command_sets = ide_ident->command_set_count;

    // (VII) Get Size:
    if (device->command_sets & (1 << 26)) {
        // Device uses 48-Bit Addressing:
        device->size = ide_ident->max_lba_upper32;
    } else {
        // Device uses CHS or 28-bit Addressing:
        device->size = ide_ident->max_lba_lower32;
    }

    // (VIII) String indicates model of device (like Western Digital HDD and
    // SONY DVD-RW...):

    // memcpy(device->model, &ide_buf[ATA_IDENT_MODEL], sizeof(device->model));
}

void
ide_init(const uint32_t bar0,
         const uint32_t bar1,
         const uint32_t bar2,
         const uint32_t bar3,
         const uint32_t bar4)
{
    g_channel_list[ATA_PRIMARY].base = (bar0 & 0xFFFFFFFC) + 0x1F0ul * !bar0;
    g_channel_list[ATA_PRIMARY].ctrl = (bar1 & 0xFFFFFFFC) + 0x3F6ul * !bar1;

    g_channel_list[ATA_SECONDARY].base = (bar2 & 0xFFFFFFFC) + 0x170ul * !bar2;
    g_channel_list[ATA_SECONDARY].ctrl = (bar3 & 0xFFFFFFFC) + 0x376ul * !bar3;

    g_channel_list[ATA_PRIMARY].bmide = bar4 & 0xFFFFFFFC;
    g_channel_list[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8;

    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
    handle_device(&g_devices_list[0], ATA_PRIMARY, ATA_MASTER_DRIVE);
    handle_device(&g_devices_list[1], ATA_PRIMARY, ATA_SLAVE_DRIVE);
    handle_device(&g_devices_list[2], ATA_SECONDARY, ATA_MASTER_DRIVE);
    handle_device(&g_devices_list[3], ATA_SECONDARY, ATA_SLAVE_DRIVE);

    static const char *const device_name_list[] = { "ATA", "ATAPI" };

    // 4- Print Summary:
    bool found_device = false;
    for (uint8_t i = 0; i != 4; i++) {
        if (g_devices_list[i].reserved != 1) {
            continue;
        }

        printk(LOGLEVEL_INFO,
               "ide: found device %s, drive " SIZE_UNIT_FMT " - %s\n",
               device_name_list[g_devices_list[i].type],
               SIZE_UNIT_FMT_ARGS(g_devices_list[i].size),
               g_devices_list[i].model);

        found_device = true;
    }

    if (!found_device) {
        printk(LOGLEVEL_WARN, "ide: no devices found\n");
    }
}

bool g_found_ide = false;

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    g_found_ide = true;

    if (!index_in_bounds(PCI_IDE_BAR_INDEX, pci_entity->max_bar_count)) {
        printk(LOGLEVEL_WARN,
               "ide: pci-device has fewer than %" PRIu32 " bars\n",
               PCI_IDE_BAR_INDEX);
        return;
    }

    struct pci_entity_bar_info *const bar =
        &pci_entity->bar_list[PCI_IDE_BAR_INDEX];

    if (!bar->is_present) {
        printk(LOGLEVEL_WARN,
               "ide: pci-device doesn't have the required bar at "
               "index %" PRIu32 "\n",
               PCI_IDE_BAR_INDEX);
        return;
    }

    if (bar->is_mmio) {
        printk(LOGLEVEL_WARN,
               "ide: pci-device's bar at index %" PRIu32 " isn't an pio bar\n",
               PCI_IDE_BAR_INDEX);
        return;
    }

    pci_entity_enable_privl(pci_entity,
                            __PCI_ENTITY_PRIVL_BUS_MASTER |
                            __PCI_ENTITY_PRIVL_PIO_ACCESS);

}

static const struct pci_driver pci_driver = {
    .init = init_from_pci,
    .match = __PCI_DRIVER_MATCH_CLASS | __PCI_DRIVER_MATCH_SUBCLASS,

    .class = PCI_ENTITY_CLASS_MASS_STORAGE_CONTROLLER,
    .subclass = PCI_ENTITY_SUBCLASS_IDE,
};

__driver static const struct driver driver = {
    .name = SV_STATIC("ide-driver"),
    .dtb = NULL,
    .pci = &pci_driver
};