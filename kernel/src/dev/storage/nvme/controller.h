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

    volatile struct nvme_registers *regs;

    struct nvme_queue admin_queue;
    struct list namespace_list;

    uint8_t stride;
    uint16_t msix_vector;

    isr_vector_t isr_vector;
};

bool
nvme_controller_create(struct nvme_controller *controller,
                       volatile struct nvme_registers *regs,
                       isr_vector_t isr_vector,
                       uint16_t msix_vector);

bool nvme_controller_destroy(struct nvme_controller *controller);
