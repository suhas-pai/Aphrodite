/*
 * kernel/src/arch/aarch64/sys/gic/its.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "asm/irqs.h"
#include "cpu/info.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "mm/phalloc.h"

#include "mm/mmio.h"
#include "mm/page_alloc.h"

#include "sys/mmio.h"
#include "its.h"

enum gic_its_baser_page_size {
    GIC_ITS_BASER_PAGE_SIZE_4KIB,
    GIC_ITS_BASER_PAGE_SIZE_16KIB,
    GIC_ITS_BASER_PAGE_SIZE_64KIB,
};

enum gic_its_baser_cacheability {
    GIC_ITS_BASER_CACHEABILITY_NONE,
    GIC_ITS_BASER_CACHEABILITY_INNER_SHAREABLE,
    GIC_ITS_BASER_CACHEABILITY_OUTER_SHAREABLE
};

enum gic_its_baser_kind {
    GIC_ITS_BASER_KIND_UNIMPLEMENTED,
    GIC_ITS_BASER_KIND_DEVICES,
    GIC_ITS_BASER_KIND_VPES,
    GIC_ITS_BASER_KIND_INT_COLLECTIONS = 0b100,
};

enum gic_its_baser_shifts {
    GIC_ITS_BASER_PAGE_SIZE_SHIFT = 8,
    GIC_ITS_BASER_ENTRY_SIZE_SHIFT_MINUS_ONE = 48,
    GIC_ITS_BASER_TYPE_SHIFT = 56
};

enum gic_its_ctrl_flags {
    __GIC_ITS_CTRL_ENABLED = 1 << 0,
};

enum gic_its_typer_shifts {
    GIC_ITS_TYPER_ITT_ENTRY_SIZE_MINUS_ONE_SHIFT = 4,
    GIC_ITS_TYPER_INT_ID_BITS_MINUS_ONE_SHIFT = 8,
    GIC_ITS_TYPER_DEV_ID_BITS_MINUS_ONE_SHIFT = 13,
    GIC_ITS_TYPER_HW_COLLECTION_COUNT_SHIFT = 24,
    GIC_ITS_TYPER_COLLECTION_ID_BITS_MINUS_ONE_SHIFT = 32,
};

enum gic_its_typer_flags {
    __GIC_ITS_TYPER_SUPPORTS_PHYS_LPIS = 1 << 0,
    __GIC_ITS_TYPER_SUPPORTS_VIRT_LPIS = 1 << 1,
    __GIC_ITS_TYPER_ITT_ENTRY_SIZE_MINUS_ONE =
        0xF << GIC_ITS_TYPER_ITT_ENTRY_SIZE_MINUS_ONE_SHIFT,
    __GIC_ITS_TYPER_INT_ID_BITS_MINUS_ONE =
        0xF << GIC_ITS_TYPER_INT_ID_BITS_MINUS_ONE_SHIFT,
    __GIC_ITS_TYPER_DEV_ID_BITS_MINUS_ONE =
        0xF << GIC_ITS_TYPER_DEV_ID_BITS_MINUS_ONE_SHIFT,
    __GIC_ITS_TYPER_SUPPORTS_SERROR_INT = 1 << 19,
    __GIC_ITS_TYPER_SUPPORTS_PHYS_TARGET_ADDRESS = 1 << 20,
    __GIC_ITS_TYPER_HW_COLLECTION_COUNT =
        0xFFull << GIC_ITS_TYPER_HW_COLLECTION_COUNT_SHIFT,
    __GIC_ITS_TYPER_COLLECTION_ID_BITS_MINUS_ONE =
        0xFull << GIC_ITS_TYPER_COLLECTION_ID_BITS_MINUS_ONE_SHIFT,
    __GIC_ITS_TYPER_SUPPORTS_COLLECTION_ID_LIMIT = 1ull << 33,
};

enum gic_its_baser_flags {
    __GIC_ITS_BASER_PAGE_COUNT = 0xFF,
    __GIC_ITS_BASER_PAGE_SIZE = 0b11 << GIC_ITS_BASER_PAGE_SIZE_SHIFT,
    __GIC_ITS_BASER_ENTRY_SIZE_MINUS_ONE =
        mask_for_n_bits(5) << GIC_ITS_BASER_ENTRY_SIZE_SHIFT_MINUS_ONE,

    __GIC_ITS_BASER_TYPE = mask_for_n_bits(7) << GIC_ITS_BASER_TYPE_SHIFT,
    __GIC_ITS_BASER_VALID = 1ull << 63,
};

struct gic_its_registers {
    volatile uint32_t control;
    volatile const uint32_t its_id;

    volatile uint64_t typer;
    volatile uint32_t waker;

    volatile const char reserved[108];
    volatile uint64_t cmd_queue_desc;

    volatile uint64_t write_reg;
    volatile uint64_t read_reg;

    volatile const char reserved_2[104];
    volatile uint64_t table_address[8];

    volatile const char reserved_3[65280];
    volatile uint64_t translater;
};

enum gic_its_command_kind {
    GIC_ITS_CMD_MOVI = 0x01,
    GIC_ITS_CMD_INT = 0x03,
    GIC_ITS_CMD_CLEAR,
    GIC_ITS_CMD_SYNC,
    GIC_ITS_CMD_MAPD = 0x08,
    GIC_ITS_CMD_MAPC,
    GIC_ITS_CMD_MAPTI,
    GIC_ITS_CMD_MAPI,
    GIC_ITS_CMD_INV,
    GIC_ITS_CMD_INVALL,
    GIC_ITS_CMD_MOVALL,
    GIC_ITS_CMD_DISCARD,
    GIC_ITS_CMD_VMOVI = 0x21,
    GIC_ITS_CMD_VMOVP,
    GIC_ITS_CMD_VSYNC = 0x25,
    GIC_ITS_CMD_VMAPP = 0x29,
    GIC_ITS_CMD_VMAPTI,
    GIC_ITS_CMD_VMAPI,
    GIC_ITS_CMD_VINVALL = 0x2D,
};

struct gic_its_cmd_queue_entry {
    enum gic_its_command_kind command;
    uint64_t dwords[3];
};

enum gic_its_device_table_entry_shifts {
    GIC_ITS_DEVICE_TABLE_ENTRY_SIZE_SHIFT = 1,
    GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR_SHIFT = 8
};

enum gic_its_device_table_entry_flags {
    __GIC_ITS_DEVICE_TABLE_ENTRY_VALID = 1 << 0,
    __GIC_ITS_DEVICE_TABLE_ENTRY_SIZE =
        mask_for_n_bits(5) << GIC_ITS_DEVICE_TABLE_ENTRY_SIZE_SHIFT,
    __GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR =
        mask_for_n_bits(44) << GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR_SHIFT,
};

struct gic_its_device_table_entry {
    uint64_t flags;
};

enum gic_its_interrupt_table_entry_shifts {
    GIC_ITS_INT_TABLE_VECTOR_SHIFT = 2,
    GIC_ITS_INT_TABLE_ICID_SHIFT = 32,
    GIC_ITS_INT_TABLE_VPEID_SHIFT = 32,
};

enum gic_its_interrupt_table_entry_flags {
    __GIC_ITS_INT_TABLE_ENTRY_VALID = 1 << 0,
    __GIC_ITS_INT_TABLE_ENTRY_PHYSICAL_INTERRUPT = 1 << 1,
    __GIC_ITS_INT_TABLE_ENTRY_VECTOR =
        mask_for_n_bits(24) << GIC_ITS_INT_TABLE_VECTOR_SHIFT,
    __GIC_ITS_INT_TABLE_ENTRY_ICID = 0xFFFFFull << GIC_ITS_INT_TABLE_ICID_SHIFT,
    __GIC_ITS_INT_TABLE_ENTRY_VPEID =
        0xFFFFFull << GIC_ITS_INT_TABLE_VPEID_SHIFT,
};

struct gic_its_interrupt_table_entry {
    uint64_t flags;
    uint32_t doorbell;
};

enum gic_its_collection_table_entry_shifts {
    GIC_ITS_COLLECTION_TABLE_ENTRY_RDBASE_SHIFT = 1,
};

enum gic_its_collection_table_entry_flags {
    __GIC_ITS_COLLECTION_TABLE_ENTRY_VALID = 1 << 0,
    __GIC_ITS_COLLECTION_TABLE_ENTRY_RDBASE =
        0xFFFFull << GIC_ITS_COLLECTION_TABLE_ENTRY_RDBASE_SHIFT
};

enum gic_its_lpi_config_table_entry_shifts {
    GIC_ITS_LPI_CONFIG_TABLE_ENTRY_PRIORITY_SHIFT = 2
};

enum gic_its_lpi_config_table_entry_flags {
    __GIC_ITS_LPI_CONFIG_TABLE_ENTRY_ENABLED = 1 << 0,
    __GIC_ITS_LPI_CONFIG_TABLE_ENTRY_PRIORITY =
        mask_for_n_bits(5) << GIC_ITS_LPI_CONFIG_TABLE_ENTRY_PRIORITY_SHIFT
};

struct gic_its_collection_table_entry {
    uint64_t flags;
};

#define GIC_MAX_ITS_INTERRUPT_TABLE_ENTRIES 32
#define GICD_DEFAULT_PRIO 0xA0

#define DEVICE_TABLE_PAGE_ORDER 7
#define COLLECTION_TABLE_PAGE_ORDER 0
#define CMD_QUEUE_PAGE_ORDER 0

static struct list g_list = LIST_INIT(g_list);
static uint32_t g_count = 0;

void send_command(struct gic_its_cmd_queue_entry *const command) {
    (void)command;
}

static bool
fill_out_collection_table(struct gic_its_info *const its, const uint16_t icid) {
    struct gic_its_collection_table_entry *const entry =
        its->int_collection_table
        + (its->int_collection_table_entry_count * icid);

    const int flag = disable_interrupts_if_not();
    entry->flags =
        this_cpu()->processor_number
            << GIC_ITS_COLLECTION_TABLE_ENTRY_RDBASE_SHIFT
        | __GIC_ITS_COLLECTION_TABLE_ENTRY_VALID;

    enable_interrupts_if_flag(flag);
    return true;
}

static bool
fill_out_device_table(struct gic_its_info *const its,
                      struct device *const device,
                      const uint16_t icid,
                      const isr_vector_t vector,
                      const uint16_t msi_index)
{
    const uint16_t id = device_get_id(device);
    struct gic_its_device_table_entry *const entry =
        its->device_table + (its->device_table_entry_size * id);

    uint64_t phys = 0;
    if ((entry->flags & __GIC_ITS_DEVICE_TABLE_ENTRY_VALID) == 0) {
        const uint64_t alloc_size =
            sizeof(struct gic_its_interrupt_table_entry)
            * GIC_MAX_ITS_INTERRUPT_TABLE_ENTRIES;

        phys = phalloc(alloc_size);
        if (phys == INVALID_PHYS) {
            return false;
        }

        entry->flags |=
            (phys >> GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR_SHIFT) << 6
            | (GIC_MAX_ITS_INTERRUPT_TABLE_ENTRIES - 1)
                << GIC_ITS_DEVICE_TABLE_ENTRY_SIZE_SHIFT
            | __GIC_ITS_DEVICE_TABLE_ENTRY_VALID;
    } else {
        phys =
            ((entry->flags & __GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR) >> 6)
                << GIC_ITS_DEVICE_TABLE_ENTRY_PHYS_ADDR_SHIFT;
    }

    struct gic_its_interrupt_table_entry *const itt = phys_to_virt(phys);
    itt[msi_index].flags =
        (uint64_t)(GIC_ITS_LPI_INTERRUPT_START + vector)
            << GIC_ITS_INT_TABLE_VECTOR_SHIFT
        | (uint64_t)icid << GIC_ITS_INT_TABLE_ICID_SHIFT
        | __GIC_ITS_INT_TABLE_ENTRY_PHYSICAL_INTERRUPT
        | __GIC_ITS_INT_TABLE_ENTRY_VALID;

    return true;
}

__optimize(3) isr_vector_t
gic_its_alloc_msi_vector(struct gic_its_info *const its,
                         struct device *const device,
                         const uint16_t msi_index)
{
    int flag = disable_interrupts_if_not();
    spin_acquire(&its->bitset_lock);

    const uint64_t vector =
        bitset_find_unset(its->bitset, /*length=*/1, /*invert=*/true);

    spin_release(&its->bitset_lock);

    const uint16_t icid = this_cpu()->icid;
    enable_interrupts_if_flag(flag);

    if (vector == BITSET_INVALID) {
        return ISR_INVALID_VECTOR;
    }

    if (!fill_out_device_table(its, device, icid, vector, msi_index)) {
        bitset_unset(its->bitset, vector);
        return ISR_INVALID_VECTOR;
    }

    if (!fill_out_collection_table(its, icid)) {
        bitset_unset(its->bitset, vector);
        return ISR_INVALID_VECTOR;
    }

    flag = disable_interrupts_if_not();
    mmio_write(&((uint8_t *)this_cpu()->gicv3_prop_page)[vector],
               __GIC_ITS_LPI_CONFIG_TABLE_ENTRY_ENABLED
               | GICD_DEFAULT_PRIO <<
                    GIC_ITS_LPI_CONFIG_TABLE_ENTRY_PRIORITY_SHIFT);

    enable_interrupts_if_flag(flag);
    return vector;
}

__optimize(3) void
gic_its_free_msi_vector(struct gic_its_info *const its,
                        const isr_vector_t vector,
                        const uint16_t msi_index)
{
    (void)msi_index;
    const int flag = spin_acquire_irq_save(&its->bitset_lock);

    bitset_unset(its->bitset, vector);
    spin_release_irq_restore(&its->bitset_lock, flag);
}

__optimize(3)
volatile uint64_t *gic_its_get_msi_address(struct gic_its_info *const its) {
    volatile struct gic_its_registers *const regs =
        (volatile struct gic_its_registers *)its->phys_addr;

    return &regs->translater;
}

struct gic_its_info *
gic_its_init_from_info(const uint32_t id, const uint64_t phys_addr) {
    struct gic_its_info *const info = kmalloc(sizeof(*info));
    if (info == NULL) {
        return NULL;
    }

    info->id = id;
    info->phys_addr = phys_addr;
    info->queue_free_slot_count =
        (PAGE_SIZE << CMD_QUEUE_PAGE_ORDER) /
        sizeof(struct gic_its_cmd_queue_entry);

    struct range range =
        RANGE_INIT(phys_addr, sizeof(struct gic_its_registers));

    if (!range_align_out(range, PAGE_SIZE, &range)) {
        printk(LOGLEVEL_WARN,
               "gic/its: failed to align register range " RANGE_FMT " to "
               "page-size\n",
               RANGE_FMT_ARGS(range));
        return NULL;
    }

    info->bitset_lock = SPINLOCK_INIT();
    info->queue_lock = SPINLOCK_INIT();
    info->mmio = vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (info->mmio == NULL) {
        kfree(info);
        printk(LOGLEVEL_WARN, "gic/its: failed to mmio-map msi registers\n");

        return false;
    }

    list_init(&info->list);
    struct page *const cmd_queue_page =
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, CMD_QUEUE_PAGE_ORDER);

    if (cmd_queue_page == NULL) {
        vunmap_mmio(info->mmio);
        kfree(info);

        printk(LOGLEVEL_WARN, "gic/its: failed to alloc cmd-queue page\n");
        return NULL;
    }

    info->bitset = kmalloc(bitset_size_for_count(GIC_ITS_MAX_LPIS_SUPPORTED));
    if (info->bitset == NULL) {
        free_page(cmd_queue_page);
        vunmap_mmio(info->mmio);
        kfree(info);

        printk(LOGLEVEL_WARN, "gic/its: failed to alloc bitset\n");
        return NULL;
    }

    volatile struct gic_its_registers *const regs = info->mmio->base;
    const uint64_t typer = mmio_read(&regs->typer);

    printk(LOGLEVEL_INFO,
           "gic/its:\n"
           "\ttyper: 0x%" PRIx64 "\n"
           "\t\tsupports physical lpis: %s\n"
           "\t\tsupports virtual lpis: %s\n"
           "\t\titt entry size: %" PRIu64 "\n"
           "\t\tint id bits: %" PRIu64 "\n"
           "\t\tdevid bits: %" PRIu64 "\n"
           "\t\tsupports serror interrupt: %s\n"
           "\t\tsupports physical target address: %s\n"
           "\t\thardware collection count: %" PRIu64 "%s\n"
           "\t\tcollection id bits: %" PRIu64 "\n"
           "\t\tsupports collection id limit: %s\n",
           typer,
           typer & __GIC_ITS_TYPER_SUPPORTS_PHYS_LPIS ? "yes" : "no",
           typer & __GIC_ITS_TYPER_SUPPORTS_VIRT_LPIS ? "yes" : "no",
           (typer & __GIC_ITS_TYPER_ITT_ENTRY_SIZE_MINUS_ONE) >>
            GIC_ITS_TYPER_ITT_ENTRY_SIZE_MINUS_ONE_SHIFT,
           ((typer & __GIC_ITS_TYPER_INT_ID_BITS_MINUS_ONE) >>
            GIC_ITS_TYPER_INT_ID_BITS_MINUS_ONE_SHIFT) + 1,
           ((typer & __GIC_ITS_TYPER_DEV_ID_BITS_MINUS_ONE) >>
            GIC_ITS_TYPER_DEV_ID_BITS_MINUS_ONE_SHIFT) + 1,
           typer & __GIC_ITS_TYPER_SUPPORTS_SERROR_INT ? "yes" : "no",
           typer & __GIC_ITS_TYPER_SUPPORTS_PHYS_TARGET_ADDRESS ? "yes" : "no",
           (typer & __GIC_ITS_TYPER_HW_COLLECTION_COUNT) >>
            GIC_ITS_TYPER_HW_COLLECTION_COUNT_SHIFT,
           ((typer & __GIC_ITS_TYPER_HW_COLLECTION_COUNT) >>
            GIC_ITS_TYPER_HW_COLLECTION_COUNT_SHIFT) != 0
                ? "" : " (memory backed)",
           ((typer & __GIC_ITS_TYPER_COLLECTION_ID_BITS_MINUS_ONE) >>
            GIC_ITS_TYPER_COLLECTION_ID_BITS_MINUS_ONE_SHIFT) + 1,
           typer & __GIC_ITS_TYPER_SUPPORTS_COLLECTION_ID_LIMIT ? "yes" : "no");

    printk(LOGLEVEL_INFO, "baser\n");
    carr_foreach_mut(regs->table_address, baser_iter) {
        uint64_t baser = *baser_iter;

        const char *page_size_desc = "unknown";
        const enum gic_its_baser_page_size page_size =
            (baser & __GIC_ITS_BASER_PAGE_SIZE)
                >> GIC_ITS_BASER_PAGE_SIZE_SHIFT;

        switch (page_size) {
            case GIC_ITS_BASER_PAGE_SIZE_4KIB:
                page_size_desc = "4kib";
                break;
            case GIC_ITS_BASER_PAGE_SIZE_16KIB:
                page_size_desc = "16kib";
                break;
            case GIC_ITS_BASER_PAGE_SIZE_64KIB:
                page_size_desc = "64kib";
                break;
        }

        const char *kind_desc = "unknown";
        const enum gic_its_baser_kind kind =
            (baser & __GIC_ITS_BASER_TYPE) >> GIC_ITS_BASER_TYPE_SHIFT;

        switch (kind) {
            case GIC_ITS_BASER_KIND_UNIMPLEMENTED:
                kind_desc = "unimplemented";
                break;
            case GIC_ITS_BASER_KIND_DEVICES:
                kind_desc = "devices";
                break;
            case GIC_ITS_BASER_KIND_VPES:
                kind_desc = "vpes";
                break;
            case GIC_ITS_BASER_KIND_INT_COLLECTIONS:
                kind_desc = "interrupt collections";
                break;
        }

        const uint16_t entry_size =
            ((baser & __GIC_ITS_BASER_ENTRY_SIZE_MINUS_ONE)
                >> GIC_ITS_BASER_ENTRY_SIZE_SHIFT_MINUS_ONE) + 1;

        printk(LOGLEVEL_INFO,
               "\tbaser %" PRIu64 "\n"
               "\t\tpage count: %" PRIu64 "\n"
               "\t\tpage size: %s\n"
               "\t\titt entry size: %" PRIu16 "\n"
               "\t\tkind: %s\n",
               baser_iter - regs->table_address,
               (baser & __GIC_ITS_BASER_PAGE_COUNT) + 1,
               page_size_desc,
               entry_size,
               kind_desc);

        baser =
            rm_mask(baser, __GIC_ITS_BASER_PAGE_SIZE)
            | GIC_ITS_BASER_PAGE_SIZE_4KIB << GIC_ITS_BASER_PAGE_SIZE_SHIFT
            | __GIC_ITS_BASER_VALID;

        mmio_write(baser_iter, baser);
        switch (kind) {
            case GIC_ITS_BASER_KIND_UNIMPLEMENTED:
                break;
            case GIC_ITS_BASER_KIND_DEVICES: {
                struct page *const table_page =
                    alloc_pages(PAGE_STATE_USED,
                                __ALLOC_ZERO,
                                DEVICE_TABLE_PAGE_ORDER);

                if (table_page == NULL) {
                    return NULL;
                }

                mmio_write(baser_iter, baser | page_to_phys(table_page));

                info->device_table = page_to_virt(table_page);
                info->device_table_entry_size = entry_size;

                break;
            }
            case GIC_ITS_BASER_KIND_VPES:
                break;
            case GIC_ITS_BASER_KIND_INT_COLLECTIONS: {
                struct page *const table_page =
                    alloc_pages(PAGE_STATE_USED,
                                __ALLOC_ZERO,
                                COLLECTION_TABLE_PAGE_ORDER);

                if (table_page == NULL) {
                    return NULL;
                }

                mmio_write(baser_iter, baser | page_to_phys(table_page));

                info->int_collection_table = page_to_virt(table_page);
                info->int_collection_table_entry_count =
                    (PAGE_SIZE << COLLECTION_TABLE_PAGE_ORDER) / entry_size;

                break;
            }
        }
    }

    mmio_write(&regs->read_reg, 0);
    mmio_write(&regs->write_reg, 0);

    mmio_write(&regs->control, __GIC_ITS_CTRL_ENABLED);
    printk(LOGLEVEL_INFO, "gic/its: fully initialized\n");

    list_add(&g_list, &info->list);
    g_count++;

    return info;
}

__optimize(3) struct list *gic_its_get_list() {
    return &g_list;
}

struct gic_its_info *
gic_its_init_from_dtb(const struct devicetree *const tree,
                      const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop_no_value *const msi_controller_prop =
        (const struct devicetree_prop_no_value *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_MSI_CONTROLLER);

    if (msi_controller_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "gic/its: dtb node is missing a 'msi-controller' property\n");

        return NULL;
    }

    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "gic/its: dtb node is missing a 'reg' property\n");

        return NULL;
    }

    if (array_item_count(reg_prop->list) != 1) {
        printk(LOGLEVEL_WARN,
               "gic/its: reg prop of dtb node is of the incorrect length\n");

        return NULL;
    }

    struct devicetree_prop_reg_info *const msi_reg_info =
        array_front(reg_prop->list);

    if (msi_reg_info->size < sizeof(struct gic_its_registers)) {
        printk(LOGLEVEL_INFO, "gic/its: reg's range is too small\n");
        return NULL;
    }

    const struct devicetree_prop_phandle *const phandle_prop =
        (const struct devicetree_prop_phandle *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

    if (phandle_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "gic/its: dtb node is missing a 'phandle' property\n");

        return NULL;
    }

    return gic_its_init_from_info(phandle_prop->phandle, msi_reg_info->address);
}
