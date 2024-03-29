/*
 * kernel/src/dev/nvme/namespace.h
 * © suhas pai
 */

#pragma once

#include "dev/storage/device.h"
#include "controller.h"

struct nvme_namespace {
    struct nvme_controller *controller;
    struct nvme_queue io_queue;

    struct list list;
    struct storage_device device;

    uint32_t nsid;
    uint32_t lba_size;

    uint64_t lba_count;
};

bool
nvme_namespace_create(struct nvme_namespace *namespace,
                      struct nvme_controller *controller,
                      uint32_t nsid,
                      uint16_t max_queue_count,
                      uint32_t max_transfer_shift);

bool
nvme_namespace_rwlba(struct nvme_namespace *namespace,
                     struct range lba_range,
                     bool write,
                     uint64_t out_phys);

void nvme_namespace_destroy(struct nvme_namespace *namespace);
