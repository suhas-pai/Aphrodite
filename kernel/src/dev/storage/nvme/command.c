/*
 * kernel/src/dev/storage/nvme/command.c
 * © suhas pai
 */

#include "controller.h"
#include "command.h"
#include "namespace.h"

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
                                         controller->msix_vector);

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