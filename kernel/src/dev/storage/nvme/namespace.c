/*
 * kernel/src/dev/nvme/namespace.h
 * © suhas pai
 */

#include "dev/printk.h"

#include "lib/size.h"
#include "lib/util.h"

#include "mm/phalloc.h"

#include "command.h"
#include "namespace.h"

#define NVME_IO_QUEUE_COUNT 1024ul

bool
nvme_namespace_create(struct nvme_namespace *const namespace,
                      struct nvme_controller *const controller,
                      const uint32_t nsid,
                      const uint16_t max_queue_entry_count,
                      const uint32_t max_transfer_shift)
{
    const uint64_t identity_phys =
        phalloc(sizeof(struct nvme_namespace_identity));

    if (identity_phys == INVALID_PHYS) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to alloc page for identify-namespace command\n");
        return false;
    }

    if (!nvme_identify(controller,
                       nsid,
                       NVME_IDENTIFY_CNS_NAMESPACE,
                       identity_phys))
    {
        phalloc_free(identity_phys);
        printk(LOGLEVEL_WARN,
               "nvme: identify-namespace command failed for nsid %" PRIu32 "\n",
               nsid);

        return false;
    }

    struct nvme_namespace_identity *const identity =
        phys_to_virt(identity_phys);

    const uint32_t formatted_lba = identity->formatted_lba_count & 0xF;
    const struct nvme_lba lba = identity->lbaf[formatted_lba];

    const uint32_t lba_size = 1ull << lba.lba_data_size_shift;
    const uint64_t lba_count = identity->size;

    printk(LOGLEVEL_INFO,
           "nvme: namespace (nsid=%" PRIu32 ") identity\n"
           "\tsize: %" PRIu64 " blocks\n"
           "\t\ttotal size: " SIZE_UNIT_FMT "\n"
           "\tcapacity: %" PRIu64 " blocks\n"
           "\t\ttotal capacity: " SIZE_UNIT_FMT "\n"
           "\tutilization: %" PRIu64 " blocks\n"
           "\t\ttotal utilization: " SIZE_UNIT_FMT "\n"
           "\tfeatures: 0x%" PRIx8 "\n"
           "\tformatted lba count: %" PRIu8 "\n",
           nsid,
           identity->size,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->size * lba_size),
           identity->capacity,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->capacity * lba_size),
           identity->utilization,
           SIZE_UNIT_FMT_ARGS_ABBREV(identity->utilization * lba_size),
           identity->features,
           identity->formatted_lba_count);

    printk(LOGLEVEL_INFO,
           "\t\tlba %" PRIu32 ":\n"
           "\t\t\tmetadata size: " SIZE_UNIT_FMT "\n"
           "\t\t\tlba data size shift: %" PRIu8 "\n"
           "\t\t\trelative performance: %" PRIu8 "\n",
           formatted_lba + 1,
           SIZE_UNIT_FMT_ARGS_ABBREV(lba.metadata_size),
           lba.lba_data_size_shift,
           lba.relative_performance);

    printk(LOGLEVEL_INFO,
           "\tformatted lba size: %" PRIu8 "\n"
           "\tmetadata capabilities: 0x%" PRIu8 "\n"
           "\tdata protection capabilities: 0x%" PRIu8 "\n"
           "\tdata protection settings: 0x%" PRIu8 "\n"
           "\tnmic: 0x%" PRIu8 "\n"
           "\treservation capabilities: 0x%" PRIu8 "\n"
           "\tformat progress indicator: 0x%" PRIu8 "\n"
           "\tdealloc lba features: 0x%" PRIu8 "\n"
           "\tnamespace atomic write unit normal: %" PRIu16 "\n"
           "\tnamespace atomic write unit power fail: %" PRIu16 "\n"
           "\tnamespace compare and write unit: %" PRIu16 "\n"
           "\tnamespace boundary size normal: %" PRIu16 "\n"
           "\tnamespace boundary size power fail: %" PRIu16 "\n"
           "\tnamespace optimal i/o boundary: %" PRIu16 "\n"
           "\tnvm capability lower: 0x%" PRIu64 "\n"
           "\tnvm capability upper: 0x%" PRIu64 "\n"
           "\tnvm preferred write granularity: %" PRIu16 "\n"
           "\tnvm preferred write alignment: %" PRIu16 "\n"
           "\tnvm preferred dealloc granularity: %" PRIu16 "\n"
           "\tnvm preferred dealloc alignment: %" PRIu16 "\n"
           "\tnamespace optimal write size: %" PRIu16 "\n"
           "\tmax single source range length: %" PRIu16 "\n"
           "\tmax copy length: %" PRIu32 "\n"
           "\tmin source range count: %" PRIu8 "\n"
           "\tattributes: %" PRIu8 "\n"
           "\tnvm set-id: %" PRIu16 "\n"
           "\tiee uid: %" PRIu64 "\n",
           identity->formatted_lba_index,
           identity->metadata_capabilities,
           identity->data_protection_capabilities,
           identity->data_protection_settings,
           identity->nmic,
           identity->reservation_capabilities,
           identity->format_progress_indicator,
           identity->dealloc_lba_features,
           identity->namespace_atomic_write_unit_normal,
           identity->namespace_atomic_write_unit_power_fail,
           identity->namespace_cmp_and_write_unit,
           identity->namespace_boundary_size_normal,
           identity->namespace_boundary_size_power_fail,
           identity->namespace_optimal_io_boundary,
           identity->nvm_capability_lower64,
           identity->nvm_capability_upper64,
           identity->namespace_pref_write_gran,
           identity->namespace_pref_write_align,
           identity->namespace_pref_dealloc_gran,
           identity->namespace_pref_dealloc_align,
           identity->namespace_optim_write_size,
           identity->max_single_source_range_length,
           identity->max_copy_length,
           identity->min_source_range_count,
           identity->attributes,
           identity->nvm_set_id,
           identity->iee_uid);

    phalloc_free(identity_phys);
    if (!nvme_queue_create(&namespace->io_queue,
                           controller,
                           /*id=*/nsid,
                           min(NVME_IO_QUEUE_COUNT, max_queue_entry_count),
                           max_transfer_shift))
    {
        return false;
    }

    if (!nvme_create_completion_queue(controller, namespace)
        || !nvme_create_submit_queue(controller, namespace))
    {
        return false;
    }

    list_init(&namespace->list);
    nvme_cache_create(&namespace->cache);

    namespace->controller = controller;
    namespace->nsid = nsid;
    namespace->lba_size = lba_size;
    namespace->lba_count = lba_count;

    list_add(&controller->namespace_list, &namespace->list);
    return true;
}

bool
nvme_namespace_rwlba(struct nvme_namespace *const namespace,
                     const struct range lba_range,
                     const bool write,
                     const uint64_t out)
{
    if (__builtin_expect(range_empty(lba_range), 0)) {
        printk(LOGLEVEL_WARN,
               "nvme: namespace got %s request with an empty lba-range\n",
               write ? "write" : "read");
        return false;
    }

    if (!index_range_in_bounds(lba_range, namespace->lba_count)) {
        printk(LOGLEVEL_WARN,
               "nvme: namespace got request to %s lba-range past end of "
               "namespace\n",
               write ? "write" : "read");
        return false;
    }

    uint64_t total_size = 0;
    if (!check_mul(namespace->lba_size, lba_range.size, &total_size)) {
        printk(LOGLEVEL_WARN,
               "nvme: namespace got request to %s lba-range past end of 64-bit "
               "range of namespace\n",
               write ? "write" : "read");
        return false;
    }

    struct nvme_command command = NVME_IO_CMD(namespace, write, out, lba_range);
    if (total_size > PAGE_SIZE) {
        command.readwrite.prp2 = out + PAGE_SIZE;
        if (total_size > (PAGE_SIZE * 2)) {
            const uint32_t prp_count = (total_size / PAGE_SIZE) - 1;
            uint64_t *const prp_list =
                &namespace->io_queue.phys_region_page_list[
                    namespace->io_queue.phys_region_pages_count *
                    command.readwrite.cid];

            for (uint32_t i = 0; i != prp_count; i++) {
                prp_list[i] = command.readwrite.prp2 + (PAGE_SIZE * i);
            }

            command.readwrite.prp2 = virt_to_phys(prp_list);
        }
    }

    return nvme_queue_submit_command(&namespace->io_queue, &command);
}

__optimize(3) static inline void *
get_block_from_cache(struct nvme_namespace *const namespace,
                     const uint64_t lba)
{
    void *block = nvme_cache_find(&namespace->cache, lba);
    if (block != NULL) {
        return block;
    }

    const uint64_t phys = phalloc(namespace->lba_size);
    if (phys == INVALID_PHYS) {
        printk(LOGLEVEL_WARN,
               "nvme: failed to alloc phys-memory while reading\n");
        return NULL;
    }

    if (!nvme_namespace_rwlba(namespace,
                              RANGE_INIT(lba, 1),
                              /*write=*/false,
                              phys))
    {
        phalloc_free(phys);
        return NULL;
    }

    block = phys_to_virt(phys);
    nvme_cache_push(&namespace->cache, lba, block);

    return block;
}

uint64_t
nvme_read(struct storage_device *const device,
          void *const buf,
          const struct range range)
{
    struct nvme_namespace *const namespace =
        container_of(device, struct nvme_namespace, device);

    uint64_t lba = range.front / namespace->lba_size;
    uint64_t lba_offset = range.front % namespace->lba_size;;

    uint64_t copy_size = namespace->lba_size - lba_offset;
    uint64_t offset = 0;

    if (__builtin_expect(offset < range.size, 1)) {
        while (true) {
            void *const block = get_block_from_cache(namespace, lba);
            if (block == NULL) {
                return offset;
            }

            const uint64_t left = range.size - offset;
            if (left < copy_size) {
                memcpy(buf + offset, block + lba_offset, left);
                break;
            }

            memcpy(buf + offset, block + lba_offset, copy_size);

            offset += copy_size;
            lba++;

            lba_offset = 0;
            copy_size = namespace->lba_size;
        }
    }

    return range.size;
}

uint64_t
nvme_write(struct storage_device *const device,
           const void *const buf,
           const struct range range)
{
    struct nvme_namespace *const namespace =
        container_of(device, struct nvme_namespace, device);

    uint64_t lba = range.front / namespace->lba_size;
    uint64_t lba_offset = range.front % namespace->lba_size;;

    uint64_t copy_size = namespace->lba_size - lba_offset;
    uint64_t offset = 0;

    const uint64_t phys = phalloc(namespace->lba_size);
    if (phys == INVALID_PHYS) {
        return 0;
    }

    void *const virt = phys_to_virt(phys);
    if (__builtin_expect(offset < range.size, 1)) {
        while (true) {
            const uint64_t left = range.size - offset;
            bool should_break = left < copy_size;

            if (should_break) {
                void *const block = get_block_from_cache(namespace, lba);
                if (block == NULL) {
                    phalloc_free(phys);
                    return offset;
                }

                if (lba_offset != 0) {
                    memcpy(virt, block, lba_offset);
                }

                memcpy(virt + lba_offset, buf + offset, left);

                const uint64_t final_part =
                    namespace->lba_size - (lba_offset + left);

                if (final_part != 0) {
                    memcpy(virt + lba_offset + left,
                           block + lba_offset + left,
                           final_part);
                }
            } else {
                memcpy(virt, buf + offset, copy_size);
            }

            if (!nvme_namespace_rwlba(namespace,
                                      RANGE_INIT(lba, 1),
                                      /*write=*/true,
                                      phys))
            {
                phalloc_free(phys);
                return offset;
            }

            if (should_break) {
                break;
            }

            offset += copy_size;
            lba++;

            lba_offset = 0;
            copy_size = namespace->lba_size;
        }
    }

    phalloc_free(phys);
    return range.size;
}

void nvme_namespace_destroy(struct nvme_namespace *const namespace) {
    const int flag = spin_acquire_with_irq(&namespace->controller->lock);

    nvme_queue_destroy(&namespace->io_queue);
    list_deinit(&namespace->list);

    spin_release_with_irq(&namespace->controller->lock, flag);

    namespace->controller = NULL;
    namespace->nsid = 0;

    namespace->lba_size = 0;
    namespace->lba_count = 0;
}