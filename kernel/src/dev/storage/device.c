/*
 * kernel/src/dev/storage/device.c
 * © suhas pai
 */

#include "lib/util.h"
#include "mm/kmalloc.h"

#include "partitions/gpt.h"
#include "partitions/mbr.h"
#include "partitions/partition.h"

static bool
parse_gpt_entries(const struct gpt_header *const header,
                  struct storage_device *const device,
                  const uint32_t lba_size,
                  const storage_device_read_t read,
                  const storage_device_write_t write)
{
    const uint32_t total_size =
        sizeof(struct gpt_entry) * header->partition_entry_count;

    const uint32_t alloc_size = min(total_size, lba_size);
    struct gpt_entry *const entry_list = kmalloc(alloc_size);

    if (entry_list == NULL) {
        return false;
    }

    uint32_t entry_list_start = header->partition_sector * SECTOR_SIZE;
    uint32_t entry_list_end = entry_list_start + total_size;

    list_init(&device->partition_list);
    do {
        const uint32_t read_size =
            min(entry_list_end - entry_list_start, alloc_size);
        const struct range entry_list_range =
            RANGE_INIT(entry_list_start, read_size);

        if (read(device, entry_list, entry_list_range) != read_size) {
            kfree(entry_list);
            return false;
        }

        const uint32_t entry_count = read_size / sizeof(struct gpt_entry);
        for (uint32_t i = 0; i != entry_count; i++) {
            const struct gpt_entry *const entry = &entry_list[i];
            if (!gpt_entry_mount_ok(entry)) {
                continue;
            }

            const uint64_t lba_count =
                entry->end - entry->start / lba_size;

            const struct range lba_range = RANGE_INIT(entry->start, lba_count);
            struct partition *const partition = kmalloc(sizeof(*partition));

            if (partition == NULL) {
                kfree(entry_list);
                return false;
            }

            if (!partition_init(partition, device, lba_range)) {
                kfree(partition);
                kfree(entry_list);

                return false;
            }

            list_add(&device->partition_list, &partition->list);
        }

        entry_list_start += read_size;
    } while (index_in_bounds(entry_list_start, entry_list_end));

    kfree(entry_list);

    device->read = read;
    device->write = write;
    device->lba_size = lba_size;

    return true;
}

static bool
parse_mbr_entries(const struct mbr_header *const header,
                  struct storage_device *const device,
                  const uint32_t lba_size,
                  const storage_device_read_t read,
                  const storage_device_write_t write)
{
    list_init(&device->partition_list);
    carr_foreach(header->entry_list, entry) {
        if (!verify_mbr_entry(entry)) {
            continue;
        }

        const struct range lba_range =
            RANGE_INIT(entry->starting_sector, entry->sector_count);
        struct partition *const partition =
            kmalloc(sizeof(*partition));

        if (partition == NULL) {
            return false;
        }

        if (!partition_init(partition, device, lba_range)) {
            kfree(partition);
            return false;
        }

        list_add(&device->partition_list, &partition->list);
    }

    device->read = read;
    device->write = write;
    device->lba_size = lba_size;

    return true;
}

bool
storage_device_create(struct storage_device *const device,
                      const uint32_t lba_size,
                      const storage_device_read_t read,
                      const storage_device_write_t write)
{
    void *const first_two_sectors = kmalloc(SECTOR_SIZE * 2);
    if (first_two_sectors == NULL) {
        return false;
    }

    if (read(device, first_two_sectors, RANGE_INIT(0, SECTOR_SIZE * 2))
            != (SECTOR_SIZE * 2))
    {
        kfree(first_two_sectors);
        return false;
    }

    const struct gpt_header *const gpt_header =
        first_two_sectors + GPT_HEADER_LOCATION;

    if (verify_gpt_header(gpt_header)) {
        const bool result =
            parse_gpt_entries(gpt_header, device, lba_size, read, write);

        kfree(first_two_sectors);
        return result;
    }

    struct mbr_header *const mbr_header =
        first_two_sectors + MBR_HEADER_LOCATION;

    if (verify_mbr_header(mbr_header)) {
        const bool result =
            parse_mbr_entries(mbr_header, device, lba_size, read, write);

        kfree(first_two_sectors);
        return result;
    }

    kfree(first_two_sectors);
    return false;
}
