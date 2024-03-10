/*
 * kernel/src/dev/nvme/device.h
 * © suhas pai
 */

#pragma once

#include "dev/nvme/queue.h"
#include "lib/list.h"
#include "sys/isr.h"

struct nvme_controller {
    struct list list;
    volatile struct nvme_registers *regs;

    struct nvme_queue admin_queue;

    uint8_t completion_queue_phase;
    uint8_t stride;

    isr_vector_t vector;
};

struct nvme_namespace {
    struct nvme_controller *controller;
    struct nvme_queue queue;

    uint32_t nsid;
};