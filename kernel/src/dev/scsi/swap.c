/*
 * kernel/src/dev/scsi/swap.c
 * © suhas pai
 */

#include <lib/endian.h>
#include "dev/scsi/swap.h"

__debug_optimize(3) void scsi_swap_data(void *const data, const uint32_t size) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    ptrrange_foreach((uint16_t *)data, data + size, iter) {
        *iter = be_to_cpu(*iter);
    }
#else
    (void)data;
    (void)size;
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */
}