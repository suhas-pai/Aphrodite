/*
 * kernel/src/dev/ata/defines.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum ata_status {
    __ATA_STATUS_REG_ERR = 1 << 0,
    __ATA_STATUS_REG_IDX = 1 << 1,
    __ATA_STATUS_REG_CORR = 1 << 2,
    __ATA_STATUS_REG_DRQ = 1 << 3,
    __ATA_STATUS_REG_DSC = 1 << 4,
    __ATA_STATUS_REG_DF = 1 << 5,
    __ATA_STATUS_REG_DRDY = 1 << 6,
    __ATA_STATUS_REG_BSY = 1 << 7,
};

enum ata_error {
    __ATA_ERROR_AMNF = 1 << 0,
    __ATA_ERROR_TK0NF = 1 << 1,
    __ATA_ERROR_ABRT = 1 << 2,
    __ATA_ERROR_MCR = 1 << 3,
    __ATA_ERROR_IDNF = 1 << 4,
    __ATA_ERROR_MC = 1 << 5,
    __ATA_ERROR_UNC = 1 << 6,
    __ATA_ERROR_BBK = 1 << 7
};

enum ata_command {
    ATA_CMD_READ_PIO = 0x20,
    ATA_CMD_READ_PIO_EXT = 0x24,
    ATA_CMD_READ_DMA = 0xC8,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_PIO = 0x30,
    ATA_CMD_WRITE_PIO_EXT = 0x34,
    ATA_CMD_WRITE_DMA = 0xCA,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
    ATA_CMD_CACHE_FLUSH = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY = 0xEC,
};

enum ata_ident_capabilities {
    __ATA_IDENTITY_CAP_DMA_SUPPORT = 1 << 8,
    __ATA_IDENTITY_CAP_LBA_SUPPORT = 1 << 9,
    __ATA_IDENTITY_CAP_IORDY_SUPPORT = 1 << 10,
    __ATA_IDENTITY_CAP_STANDBY_TIMER_SUPPORTED = 1 << 13,
};

enum ata_ident_capabilities_ext {
    __ATA_IDENTITY_CAP_EXT_HAS_MIN_STANDBY_TIME = 1 << 0,
};

enum ata_ident_sata_cap_flags {
    __ATA_IDENTITY_SATA_CAP_GEN1 = 1 << 1,
    __ATA_IDENTITY_SATA_CAP_GEN2 = 1 << 2,
    __ATA_IDENTITY_SATA_CAP_GEN3 = 1 << 3,
    __ATA_IDENTITY_SATA_CAP_SUPPORTS_NCQ = 1 << 8,
};

struct ata_identity {
    uint16_t device_type;
    uint16_t cylinder_count;

    const char reserved[2];

    uint16_t head_count;
    char reserved_2[4];

    uint16_t sector_count;

    char serial[20];
    const char reserved_3[12];

    uint64_t firmware_version;

    char model[40];
    const char reserved_4[4];

    uint16_t capabilities;
    uint16_t capabilities_ext;
    const char reserved_5[18];

    uint32_t max_lba_lower32;
    const char reserved_6[28];

    uint16_t sata_capabilities;
    uint16_t sata_capabilities_ext;

    const char reserved_7[8];

    uint32_t command_set_count;
    const char reserved_8[32];

    uint32_t max_lba_upper32;
} __packed;

enum atapi_identity_fieldvalid_flags {
    __ATAPI_IDENTITY_FIELDVALID_64_70_VALID = 1 << 0,
    __ATAPI_IDENTITY_FIELDVALID_88_VALID = 1 << 1,
};

struct atapi_identify {
    uint16_t device_type;
    const char reserved[18];

    char serial[20];

    uint16_t buffer_kind;
    uint16_t cache_size;
    uint16_t ecc;

    char version[8];
    char model[40];

    uint16_t dword_flags;
    uint16_t flags;

    uint16_t capabilities;
    char reserved_2[6];

    uint16_t fieldvalid;
} __packed;

enum ata_register {
    ATA_REG_DATA,
    ATA_REG_ERROR,
    ATA_REG_FEATURES,
    ATA_REG_SECCOUNT0,
    ATA_REG_LBA0,
    ATA_REG_LBA1,
    ATA_REG_LBA2,
    ATA_REG_HDDEVSEL,
    ATA_REG_COMMAND,
    ATA_REG_STATUS,
    ATA_REG_SECCOUNT1,
    ATA_REG_LBA3,
    ATA_REG_LBA4,
    ATA_REG_LBA5,
    ATA_REG_CONTROL,
    ATA_REG_ALT_STATUS,
    ATA_REG_DEV_ADDRESS,
};

enum {
    __ATA_USE_LBA_ADDRESSING = 1 << 6
};

#define ATA_SECTOR_SIZE 512
