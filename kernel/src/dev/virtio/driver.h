/*
 * kernel/src/dev/virtio/driver.h
 * © suhas pai
 */

#pragma once

#include "drivers/block.h"
#include "drivers/scsi.h"

#include "device.h"

typedef struct virtio_device *
(*virtio_driver_init_t)(struct virtio_device *device, uint64_t features);

struct virtio_driver_info {
    virtio_driver_init_t init;

    uint16_t virtqueue_count;
    uint64_t required_features;
};

static const struct virtio_driver_info virtio_drivers[] = {
    [VIRTIO_DEVICE_KIND_BLOCK_DEVICE] = {
        .init = virtio_block_driver_init,
        .virtqueue_count = 2,
        .required_features =
            __VIRTIO_BLOCK_HAS_SEG_MAX |
            __VIRTIO_BLOCK_HAS_BLOCK_SIZE |
            __VIRTIO_BLOCK_SUPPORTS_MULTI_QUEUE,
    },
    [VIRTIO_DEVICE_KIND_SCSI_HOST] = {
        .init = virtio_scsi_driver_init,
        .virtqueue_count = 0,
        .required_features = __VIRTIO_SCSI_HOTPLUG | __VIRTIO_SCSI_CHANGE,
    }
};

static const struct string_view virtio_device_kind_string[] = {
    [VIRTIO_DEVICE_KIND_INVALID] = SV_STATIC("reserved"),
    [VIRTIO_DEVICE_KIND_NETWORK_CARD] = SV_STATIC("network-card"),
    [VIRTIO_DEVICE_KIND_BLOCK_DEVICE] = SV_STATIC("block-device"),
    [VIRTIO_DEVICE_KIND_CONSOLE] = SV_STATIC("entropy-source"),
    [VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD] = SV_STATIC("memory-balloon-trad"),
    [VIRTIO_DEVICE_KIND_IOMEM] = SV_STATIC("iomem"),
    [VIRTIO_DEVICE_KIND_RPMSG] = SV_STATIC("rpmsg"),
    [VIRTIO_DEVICE_KIND_SCSI_HOST] = SV_STATIC("scsi-host"),
    [VIRTIO_DEVICE_KIND_9P_TRANSPORT] = SV_STATIC("9p-transport"),
    [VIRTIO_DEVICE_KIND_MAC_80211_WLAN] = SV_STATIC("mac80211-wlan"),
    [VIRTIO_DEVICE_KIND_RPROC_SERIAL] = SV_STATIC("rproc-serial"),
    [VIRTIO_DEVICE_KIND_VIRTIO_CAIF] = SV_STATIC("virtio-caif"),
    [VIRTIO_DEVICE_KIND_MEM_BALLOON] = SV_STATIC("mem-baloon"),
    [VIRTIO_DEVICE_KIND_GPU_DEVICE] = SV_STATIC("gpu-device"),
    [VIRTIO_DEVICE_KIND_TIMER_OR_CLOCK] = SV_STATIC("timer-or-clock"),
    [VIRTIO_DEVICE_KIND_INPUT_DEVICE] = SV_STATIC("input-device"),
    [VIRTIO_DEVICE_KIND_SOCKET_DEVICE] = SV_STATIC("socker-device"),
    [VIRTIO_DEVICE_KIND_CRYPTO_DEVICE] = SV_STATIC("crypto-device"),
    [VIRTIO_DEVICE_KIND_SIGNAL_DISTR_NODULE] =
        SV_STATIC("signal-distribution-module"),
    [VIRTIO_DEVICE_KIND_PSTORE_DEVICE] = SV_STATIC("pstore-device"),
    [VIRTIO_DEVICE_KIND_IOMMU_DEVICE] = SV_STATIC("iommu-device"),
    [VIRTIO_DEVICE_KIND_MEMORY_DEVICE] = SV_STATIC("audio-device"),
    [VIRTIO_DEVICE_KIND_FS_DEVICE] = SV_STATIC("fs-device"),
    [VIRTIO_DEVICE_KIND_PMEM_DEVICE] = SV_STATIC("pmem-device"),
    [VIRTIO_DEVICE_KIND_RPMB_DEVICE] = SV_STATIC("rpmb-device"),
    [VIRTIO_DEVICE_KIND_MAC_80211_HWSIM_WIRELESS_SIM_DEVICE] =
        SV_STATIC("mac80211-hwsim-wireless-simulator-device"),
    [VIRTIO_DEVICE_KIND_VIDEO_ENCODER_DEVICE] =
        SV_STATIC("video-encoder-device"),
    [VIRTIO_DEVICE_KIND_VIDEO_DECODER_DEVICE] =
        SV_STATIC("video-decoder-device"),
    [VIRTIO_DEVICE_KIND_SCMI_DEVICE] = SV_STATIC("scmi-device"),
    [VIRTIO_DEVICE_KIND_NITRO_SECURE_MDOEL] = SV_STATIC("nitro-secure-model"),
    [VIRTIO_DEVICE_KIND_I2C_ADAPTER] = SV_STATIC("i2c-adapter"),
    [VIRTIO_DEVICE_KIND_WATCHDOG] = SV_STATIC("watchdog"),
    [VIRTIO_DEVICE_KIND_CAN_DEVICE] = SV_STATIC("can-device"),
    [VIRTIO_DEVICE_KIND_PARAM_SERVER] = SV_STATIC("parameter-server"),
    [VIRTIO_DEVICE_KIND_AUDIO_POLICY_DEVICE] = SV_STATIC("audio-policy-device"),
    [VIRTIO_DEVICE_KIND_BLUETOOTH_DEVICE] = SV_STATIC("bluetooth-device"),
    [VIRTIO_DEVICE_KIND_GPIO_DEVICE] = SV_STATIC("gpio-device"),
    [VIRTIO_DEVICE_KIND_RDMA_DEVICE] = SV_STATIC("rdma-device")
};