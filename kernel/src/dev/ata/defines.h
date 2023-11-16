/*
 * kernel/src/dev/ata/defines.h
 * © suhas pai
 */

#pragma once

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
    ATA_CMD_READ_PIO        = 0x20,
    ATA_CMD_READ_PIO_EXT    = 0x24,
    ATA_CMD_READ_DMA        = 0xC8,
    ATA_CMD_READ_DMA_EXT    = 0x25,
    ATA_CMD_WRITE_PIO       = 0x30,
    ATA_CMD_WRITE_PIO_EXT   = 0x34,
    ATA_CMD_WRITE_DMA       = 0xCA,
    ATA_CMD_WRITE_DMA_EXT   = 0x35,
    ATA_CMD_CACHE_FLUSH     = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET          = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY        = 0xEC,
};

enum atapi_commands {
    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B,
};

enum ata_identity_f {
    ATA_IDENT_DEVICE_TYPE,
    ATA_IDENT_CYLINDERS = 2,
    ATA_IDENT_HEADS = 6,
    ATA_IDENT_SECTORS = 12,
    ATA_IDENT_SERIAL = 20,
    ATA_IDENT_MODEL = 54,
    ATA_IDENT_CAPABILITIES = 98,
    ATA_IDENT_FIELD_VALID = 106,
    ATA_IDENT_MAX_LBA = 120,
    ATA_IDENT_COMMAND_SETS = 164,
    ATA_IDENT_MAX_LBA_EXT = 200,
};

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