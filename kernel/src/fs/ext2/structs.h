/*
 * kernel/src/fs/ext2/structs.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum ext2fs_state {
    EXT2FS_STATE_CLEAN = 1,
    EXT2FS_STATE_HAS_ERRORS,
};

enum ext2fs_error_handling_strategy {
    EXT2FS_ERROR_HANDLING_IGNORE,
    EXT2FS_ERROR_HANDLING_FS_READ_ONLY,
    EXT2FS_ERROR_HANDLING_KERNEL_PANIC,
};

enum ext2fs_os_id {
    EXT2FS_OS_ID_LINUX,
    EXT2FS_OS_ID_GNU_HURD,
    EXT2FS_OS_ID_MASIX,
    EXT2FS_OS_ID_FREEBSD,
    EXT2FS_OS_ID_OTHER,
};

enum ext2fs_optional_features {
    __EXT2FS_OPT_FEAT_PREALLOC_BLOCKS = 1 << 0,
    __EXT2FS_OPT_FEAT_AFS_SERVER_INODES_EXIST = 1 << 1,
    __EXT2FS_OPT_FEAT_JOURNALING = 1 << 2,
    __EXT2FS_OPT_FEAT_INODE_EXT_ATTR = 1 << 3,
    __EXT2FS_OPT_FEAT_FS_CAN_RESIZE = 1 << 4,
    __EXT2FS_OPT_FEAT_DIR_USE_HASH_INDEX = 1 << 5,
};

enum ext2fs_required_features {
    __EXT2FS_REQ_FEAT_COMPRESSION = 1 << 0,
    __EXT2FS_REQ_FEAT_DIR_ENTRIES_TYPE_FIELD = 1 << 1,
    __EXT2FS_REQ_FEAT_JOURNAL_REPLAY = 1 << 2,
    __EXT2FS_REQ_FEAT_USES_JOURNAL_DEVICE = 1 << 3,
};

enum ext2fs_readonly_features {
    __EXT2FS_READONLY_FEAT_SPARSE_SUPERBLOCKS_AND_GROUP_DESC = 1 << 0,
    __EXT2FS_READONLY_FEAT_64BIT_FILE_SIZE = 1 << 1,
    __EXT2FS_READONLY_FEAT_DIR_ENTRIES_IN_BINARY_TREE = 1 << 2,
};

#define EXT2FS_SUPERBLOCK_LOC 1024
#define EXT2FS_SUPERBLOCK_SIGNATURE 0xEF53

struct ext2fs_superblock {
    uint32_t inode_count;
    uint32_t block_count;

    uint32_t superblock_reserved_count;

    uint32_t unallocated_block_count;
    uint32_t unallocated_inode_count;
    uint32_t superblock_block_number;

    uint32_t block_size; // In log2 (block size) - 10
    uint32_t fragment_size; // In log2 (fragment size) - 10

    uint32_t blocks_per_group_count;
    uint32_t fragments_per_group_count;
    uint32_t inodes_per_group_count;

    uint32_t last_mount_epoch; // unix epoch for last mount
    uint32_t last_written_epoch; // unix epoch for last write

    uint16_t mount_count;
    uint16_t mount_count_before_fsck;

    uint16_t signature;
    uint16_t fs_state;

    uint16_t error_strategy;
    uint16_t version_minor;
    uint32_t last_fsck_epoch;
    uint32_t forced_fsck_interval;
    uint32_t os_id;
    uint32_t verion_major;
    uint16_t user_id;
    uint16_t group_id;

    struct {
        uint32_t first_usable_inode;
        uint16_t inode_size;
        uint16_t superblock_block_group;
        uint32_t optional_features;
        uint32_t required_features;
        uint32_t readonly_features;

        char uuid[16];
        char volume_name[16];
        char lastmountedpath[64]; // last path we had when mounted

        uint32_t compresion_algos;

        uint8_t file_prealloc_block_count;
        uint8_t directory_prealloc_block_count;

        const uint16_t reserved;
        char journal_id[16];

        uint32_t journal_inode;
        uint32_t journal_device;

        uint32_t orphan_inode_head;
    } extended;
} __packed;
