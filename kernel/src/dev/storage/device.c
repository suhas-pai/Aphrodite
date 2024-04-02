/*
 * kernel/src/dev/storage/device.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/phalloc.h"

#include "partitions/gpt.h"
#include "partitions/mbr.h"
#include "partitions/partition.h"

#include "fs/driver.h"

static bool
parse_gpt_entries(struct storage_device *const device,
                  const struct gpt_header *const header)
{
    const uint32_t total_size =
        sizeof(struct gpt_entry) * header->partition_entry_count;

    const uint32_t alloc_size = min(total_size, device->lba_size);
    struct gpt_entry *const entry_list = kmalloc(alloc_size);

    if (entry_list == NULL) {
        return false;
    }

    uint32_t entry_list_start = header->partition_sector * SECTOR_SIZE;
    const uint32_t entry_list_end = entry_list_start + total_size;

    list_init(&device->partition_list);
    bool found_atleast_one = false;

    do {
        const uint32_t read_size =
            min(entry_list_end - entry_list_start, device->lba_size);
        const struct range entry_list_range =
            RANGE_INIT(entry_list_start, read_size);

        if (storage_device_read(device, entry_list, entry_list_range)
                != read_size)
        {
            kfree(entry_list);
            return false;
        }

        const uint32_t entry_count = read_size / sizeof(struct gpt_entry);
        for (uint32_t i = 0; i != entry_count; i++) {
            const struct gpt_entry *const entry = &entry_list[i];
            if (!gpt_entry_mount_ok(entry)) {
                continue;
            }

            const uint64_t lba_count = entry->end - entry->start;
            if (lba_count == 0) {
                continue;
            }

            const struct range lba_range = RANGE_INIT(entry->start, lba_count);
            struct range full_range = RANGE_EMPTY();

            if (!range_multiply(lba_range, device->lba_size, &full_range)) {
                continue;
            }

            struct partition *const partition = kmalloc(sizeof(*partition));
            if (partition == NULL) {
                kfree(entry_list);
                return false;
            }

            if (!partition_init(partition, device, full_range)) {
                kfree(partition);
                continue;
            }

            list_add(&device->partition_list, &partition->list);
            found_atleast_one = true;
        }

        entry_list_start += read_size;
    } while (index_in_bounds(entry_list_start, entry_list_end));

    kfree(entry_list);
    return found_atleast_one;
}

static bool
parse_mbr_entries(struct storage_device *const device,
                  const struct mbr_header *const header)
{
    list_init(&device->partition_list);

    bool found_atleast_one = false;
    carr_foreach(header->entry_list, entry) {
        if (!verify_mbr_entry(entry)) {
            continue;
        }

        const struct range lba_range =
            RANGE_INIT(entry->starting_sector, entry->sector_count);

        struct range full_range = RANGE_EMPTY();
        if (!range_multiply(lba_range, device->lba_size, &full_range)) {
            continue;
        }

        struct partition *const partition = kmalloc(sizeof(*partition));
        if (partition == NULL) {
            return false;
        }

        if (!partition_init(partition, device, full_range)) {
            kfree(partition);
            continue;
        }

        list_add(&device->partition_list, &partition->list);
        found_atleast_one = true;
    }

    return found_atleast_one;
}

static bool identify_partitions(struct storage_device *const device) {
    const struct range gpt_range =
        RANGE_INIT(GPT_HEADER_LOCATION, sizeof(struct gpt_header));

    struct gpt_header gpt_header;
    if (storage_device_read(device, &gpt_header, gpt_range) != gpt_range.size) {
        return false;
    }

    if (verify_gpt_header(&gpt_header)) {
        return parse_gpt_entries(device, &gpt_header);
    }

    const struct range mbr_range =
        RANGE_INIT(MBR_HEADER_LOCATION, sizeof(struct mbr_header));

    struct mbr_header mbr_header;
    if (storage_device_read(device, &mbr_header, mbr_range) != mbr_range.size) {
        return false;
    }

    if (verify_mbr_header(&mbr_header)) {
        return parse_mbr_entries(device, &mbr_header);
    }

    return true;
}

bool
storage_device_init(struct storage_device *const device,
                    const uint32_t lba_size,
                    const storage_device_read_t read,
                    const storage_device_write_t write)
{
    storage_cache_create(&device->cache);

    device->read = read;
    device->write = write;
    device->lba_size = lba_size;

    if (!identify_partitions(device)) {
        printk(LOGLEVEL_WARN,
               "storage: failed to identify any partitions in device\n");
        return false;
    }

    struct partition *iter = NULL;

    uint8_t partition_index = 0;
    bool found_atleast_one_fs = false;

    list_foreach(iter, &device->partition_list, list) {
        bool found_fs = false;
        fs_driver_foreach(driver) {
            if (driver->try_init(iter)) {
                printk(LOGLEVEL_INFO,
                       "storage: found " SV_FMT " partition\n",
                       SV_FMT_ARGS(driver->name));

                found_fs = true;
                break;
            }
        }

        if (found_fs) {
            found_atleast_one_fs = true;
            partition_index++;

            continue;
        }

        printk(LOGLEVEL_WARN,
               "storage: partition #%" PRIu8 " has an unknown filesystem\n",
               partition_index);

        partition_index++;
    }

    return found_atleast_one_fs;
}

__optimize(3) static inline void *
find_in_cache_or_read_block(struct storage_device *const device,
                            const uint64_t lba)
{
    void *block = storage_cache_find(&device->cache, lba);
    if (block != NULL) {
        return block;
    }

    const uint64_t phys = phalloc(device->lba_size);
    if (phys == INVALID_PHYS) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to alloc phys-memory while reading\n");
        return NULL;
    }

    if (device->read(device, phys, RANGE_INIT(lba, 1)) != 1) {
        phalloc_free(phys);
        return NULL;
    }

    block = phys_to_virt(phys);
    storage_cache_push(&device->cache, lba, block);

    return block;
}

uint64_t
storage_device_read(struct storage_device *const device,
                    void *const buf,
                    const struct range range)
{
    uint64_t lba = range.front / device->lba_size;
    uint64_t block_offset = range.front % device->lba_size;
    uint64_t buf_offset = 0;

    if (__builtin_expect(buf_offset < range.size, 1)) {
        while (true) {
            void *const block = find_in_cache_or_read_block(device, lba);
            if (block == NULL) {
                return buf_offset;
            }

            const uint64_t left = range.size - buf_offset;
            uint64_t copy_size = device->lba_size - block_offset;

            if (left < copy_size) {
                memcpy(buf + buf_offset, block + block_offset, left);
                break;
            }

            memcpy(buf + buf_offset, block + block_offset, copy_size);

            buf_offset += copy_size;
            lba++;

            block_offset = 0;
        }
    }

    return range.size;
}

uint64_t
storage_device_write(struct storage_device *const device,
                     const void *const buf,
                     const struct range range)
{
    uint64_t lba = range.front / device->lba_size;
    uint64_t lba_offset = range.front % device->lba_size;

    uint64_t copy_size = device->lba_size - lba_offset;
    uint64_t offset = 0;

    const uint64_t phys = phalloc(SECTOR_SIZE);
    if (phys == INVALID_PHYS) {
        return 0;
    }

    void *const virt = phys_to_virt(phys);
    if (__builtin_expect(offset < range.size, 1)) {
        while (true) {
            const uint64_t left = range.size - offset;
            bool should_break = left < copy_size;

            if (should_break) {
                void *const block = find_in_cache_or_read_block(device, lba);
                if (block == NULL) {
                    phalloc_free(phys);
                    return offset;
                }

                if (lba_offset != 0) {
                    memcpy(virt, block, lba_offset);
                }

                memcpy(virt + lba_offset, buf + offset, left);

                const uint64_t final_part = SECTOR_SIZE - (lba_offset + left);
                if (final_part != 0) {
                    memcpy(virt + lba_offset + left,
                           block + lba_offset + left,
                           final_part);
                }
            } else {
                memcpy(virt + lba_offset, buf + offset, copy_size);
            }

            if (device->write(device, phys, RANGE_INIT(lba, 1)) != 1) {
                phalloc_free(phys);
                return offset;
            }

            if (should_break) {
                break;
            }

            offset += copy_size;
            lba++;

            lba_offset = 0;
            copy_size = SECTOR_SIZE;
        }
    }

    phalloc_free(phys);
    return range.size;
}
