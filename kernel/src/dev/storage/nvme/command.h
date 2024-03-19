/*
 * kernel/src/dev/nvme/command.h
 * © suhas pai
 */

#pragma once

#include "queue.h"
#include "structs.h"

#define NVME_IDENTIFY_CMD(queue, nsid_, cns_, out) \
    ((struct nvme_command){ \
        .identify = { \
            .opcode = NVME_CMD_ADMIN_OPCODE_IDENTIFY, \
            .cid = nvme_queue_get_cmdid(queue), \
            .nsid = (nsid_), \
            .cns = (cns_), \
            .prp1 = out, \
            .prp2 = 0, \
        } \
    })

#define NVME_CREATE_SUBMIT_QUEUE_CMD(controller, queue) \
    ((struct nvme_command){ \
        .create_submit_queue = { \
            .opcode = NVME_CMD_ADMIN_OPCODE_CREATE_SQ, \
            .cid = nvme_queue_get_cmdid(&(controller)->admin_queue), \
            .prp1 = (queue)->submit_queue_phys, \
            .sqid = (queue)->id, \
            .cqid = (queue)->id, \
            .size = (queue)->entry_count - 1, \
            .sqflags = __NVME_CREATE_SQ_PHYS_CONTIG, \
        } \
    })

#define NVME_CREATE_COMPLETION_QUEUE_CMD(controller, queue, vector) \
    ((struct nvme_command){ \
        .create_comp_queue = { \
            .opcode = NVME_CMD_ADMIN_OPCODE_CREATE_CQ, \
            .cid = nvme_queue_get_cmdid(&(controller)->admin_queue), \
            .prp1 = (queue)->completion_queue_phys, \
            .cqid = (queue)->id, \
            .size = (queue)->entry_count - 1, \
            .cqflags = \
                __NVME_CREATE_CQ_PHYS_CONTIG \
                | __NVME_CREATE_CQ_IRQS_ENABLED, \
            .irqvec = (vector) \
        } \
    })

#define NVME_SET_FEATURES_CMD(fid_, dword_, prp1_, prp2_) \
    ((struct nvme_command){ \
        .features = { \
            .opcode = NVME_CMD_ADMIN_OPCODE_SETFT, \
            .fid = (fid_), \
            .prp1 = (prp1_), \
            .prp2 = (prp2_), \
            .dword = (dword_) \
        } \
    })

#define NVME_IO_CMD(namespace, write, buf_phys, lba_range) \
    ((struct nvme_command){ \
        .readwrite = { \
            .opcode = (write) ? NVME_CMD_OPCODE_WRITE : NVME_CMD_OPCODE_READ, \
            .flags = 0, \
            .cid = nvme_queue_get_cmdid(&namespace->io_queue), \
            .nsid = (namespace)->nsid, \
            .metadata = 0, \
            .prp1 = (buf_phys), \
            .prp2 = 0, \
            .slba = (lba_range).front, \
            .len = (lba_range).size, \
            .control = 0, \
            .dsmgmt = 0, \
            .ref = 0, \
            .apptag = 0, \
            .appmask = 0, \
        } \
    })

