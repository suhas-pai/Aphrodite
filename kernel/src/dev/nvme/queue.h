/*
 * kernel/src/dev/nvme/queue.h
 * © suhas pai
 */

#pragma once

#include "sched/event.h"
#include "structs.h"

struct nvme_queue {
    struct nvme_controller *controller;

    struct page *submit_queue_pages;
    struct page *completion_queue_pages;

    struct mmio_region *submit_queue_mmio;
    struct mmio_region *completion_queue_mmio;

    struct event free_slot_event;
    struct spinlock lock;

    volatile uint32_t *submit_doorbell;
    uint64_t *phys_region_pages;

    uint8_t id;
    uint8_t submit_queue_head;
    uint8_t completion_queue_head;
    uint8_t submit_queue_tail;

    uint16_t entry_count;
    uint16_t cmd_identifier;

    uint8_t submit_alloc_order;
    uint8_t completion_alloc_order;

    bool phase : 1;
    uint32_t phys_region_pages_count;
};

struct nvme_controller;

bool
nvme_queue_create(struct nvme_queue *queue,
                  struct nvme_controller *device,
                  uint8_t id,
                  uint16_t entry_count,
                  uint8_t max_transfer_shift);

void nvme_queue_destroy(struct nvme_queue *queue);

bool
nvme_queue_submit_command(struct nvme_queue *queue,
                          struct nvme_command *command);
