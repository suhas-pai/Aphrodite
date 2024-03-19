/*
 * kernel/src/dev/nvme/device.h
 * © suhas pai
 */

#pragma once

#include "sys/isr.h"
#include "queue.h"

#define NVME_ADMIN_QUEUE_COUNT 32

struct nvme_controller {
    struct list list;
    struct spinlock lock;

    struct pci_entity_info *pci_entity;
    volatile struct nvme_registers *regs;

    struct nvme_queue admin_queue;
    struct list namespace_list;

    uint8_t stride;
    uint16_t msix_vector;

    isr_vector_t isr_vector;
};

bool
nvme_controller_create(struct nvme_controller *controller,
                       struct pci_entity_info *pci_entity,
                       volatile struct nvme_registers *regs,
                       uint8_t stride,
                       uint16_t msix_vector);

bool
nvme_identify(struct nvme_controller *controller,
              uint32_t nsid,
              enum nvme_identify_cns cns,
              uint64_t out);

struct nvme_namespace;

bool
nvme_create_submit_queue(struct nvme_controller *controller,
                         const struct nvme_namespace *namespace);

bool
nvme_create_completion_queue(struct nvme_controller *controller,
                             const struct nvme_namespace *namespace);

bool
nvme_set_number_of_queues(struct nvme_controller *controller,
                          uint16_t queue_count);

bool nvme_controller_destroy(struct nvme_controller *controller);
