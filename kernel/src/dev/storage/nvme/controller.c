/*
 * kernel/src/dev/nvme/controller.c
 * © suhas pai
 */

#include "cpu/isr.h"
#include "dev/printk.h"
#include "mm/kmalloc.h"

#include "command.h"
#include "controller.h"
#include "namespace.h"

bool
nvme_controller_create(struct nvme_controller *const controller,
                       volatile struct nvme_registers *const regs,
                       const uint8_t stride)
{
    list_init(&controller->list);
    list_init(&controller->namespace_list);

    controller->regs = regs;
    controller->stride = stride;
    controller->vector = isr_alloc_vector(/*for_msi=*/true);
    controller->lock = SPINLOCK_INIT();

    if (controller->vector == ISR_INVALID_VECTOR) {
        isr_free_vector(controller->vector, /*for_msi=*/false);
        printk(LOGLEVEL_WARN, "nvme: failed to allocate queue\n");

        return false;
    }

    if (!nvme_queue_create(&controller->admin_queue,
                           controller,
                           /*id=*/0,
                           NVME_ADMIN_QUEUE_COUNT,
                           /*max_transfer_shift=*/0))
    {
        isr_free_vector(controller->vector, /*for_msi=*/false);
        return false;
    }

    return true;
}

bool
nvme_identify(struct nvme_controller *const controller,
              const uint32_t nsid,
              const enum nvme_identify_cns cns,
              const uint64_t out)
{
    const struct nvme_command command =
        NVME_IDENTIFY_CMD(&controller->admin_queue, nsid, cns, out);

    return nvme_queue_submit_command(&controller->admin_queue, &command);
}

bool
nvme_create_submit_queue(struct nvme_controller *const controller,
                         const struct nvme_namespace *const namespace)
{
    const struct nvme_command submit_command =
        NVME_CREATE_SUBMIT_QUEUE_CMD(controller, &namespace->io_queue);

    return nvme_queue_submit_command(&controller->admin_queue, &submit_command);
}

bool
nvme_create_completion_queue(struct nvme_controller *const controller,
                             const struct nvme_namespace *const namespace)
{
    const struct nvme_command comp_command =
        NVME_CREATE_COMPLETION_QUEUE_CMD(controller,
                                         &namespace->io_queue,
                                         controller->vector);

    return nvme_queue_submit_command(&controller->admin_queue, &comp_command);
}

bool
nvme_set_number_of_queues(struct nvme_controller *const controller,
                          const uint16_t queue_count)
{
    const struct nvme_command setft_command =
        NVME_SET_FEATURES_CMD(NVME_CMD_FEATURE_QUEUE_COUNT,
                              (uint32_t)queue_count << 16 | queue_count,
                              /*prp1=*/0,
                              /*prp2=*/0);

    return nvme_queue_submit_command(&controller->admin_queue, &setft_command);
}

bool nvme_controller_destroy(struct nvme_controller *const controller) {
    spinlock_deinit(&controller->lock);

    struct nvme_namespace *iter = NULL;
    list_foreach(iter, &controller->namespace_list, list) {
        nvme_namespace_destroy(iter);
    }

    list_deinit(&controller->list);
    list_deinit(&controller->namespace_list);

    isr_free_vector(controller->vector, /*for_msi=*/true);

    controller->regs = NULL;
    controller->stride = 0;
    controller->vector = 0;

    kfree(controller);
    return true;
}
