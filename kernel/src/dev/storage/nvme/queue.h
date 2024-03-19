/*
 * kernel/src/dev/nvme/queue.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "mm/mmio.h"

#include "sched/event.h"
#include "structs.h"

struct nvme_queue_doorbells {
    volatile uint32_t submit;
    volatile uint32_t complete;
};

struct nvme_controller;
struct nvme_queue {
    struct nvme_controller *controller;

    struct spinlock lock;
    struct event event;

    uint64_t submit_queue_phys;
    uint64_t completion_queue_phys;

    struct mmio_region *submit_queue_mmio;
    struct mmio_region *completion_queue_mmio;

    volatile struct nvme_queue_doorbells *doorbells;

    uint8_t id;
    uint8_t submit_queue_head;
    uint8_t completion_queue_head;
    uint8_t submit_queue_tail;

    uint16_t entry_count;
    uint16_t cmd_identifier;

    bool phase : 1;

    uint32_t phys_region_pages_count;
    uint64_t *phys_region_page_list;
};

struct nvme_controller;

bool
nvme_queue_create(struct nvme_queue *queue,
                  struct nvme_controller *device,
                  uint8_t id,
                  uint16_t entry_count,
                  uint8_t max_transfer_shift);

uint16_t nvme_queue_get_cmdid(struct nvme_queue *queue);

bool
nvme_queue_submit_command(struct nvme_queue *queue,
                          const struct nvme_command *command);

void nvme_queue_destroy(struct nvme_queue *queue);
