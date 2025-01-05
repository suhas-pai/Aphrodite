/*
 * kernel/src/sys/uacpi.c
 * © suhas pai
 */

#include <stdatomic.h>
#include <uacpi/kernel_api.h>

#include "dev/pci/ecam.h"

#include "cpu/isr.h"
#include "cpu/mutex.h"

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/util.h"

#include "mm/kmalloc.h"

#include "mm/memmap.h"
#include "mm/mmio.h"

#include "mm/simple_alloc.h"

#include "sched/sleep.h"
#include "sched/thread.h"

#include "sys/boot.h"
#include "sys/pio.h"

#include "time/time.h"

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *const out_rsdp_address) {
    *out_rsdp_address = virt_to_phys(boot_get_rsdp());
    return UACPI_STATUS_OK;
}

struct uacpi_pci_handle {
    const struct pci_domain *domain;
    uacpi_pci_address address;
};

struct uacpi_pci_handle *
uacpi_pci_handle_create(const struct pci_domain *const domain,
                        const uacpi_pci_address address)
{
    struct uacpi_pci_handle *const handle = uacpi_kernel_alloc(sizeof(*handle));
    if (handle == nullptr) {
        return UACPI_NULL;
    }

    handle->domain = domain;
    handle->address = address;

    return handle;
}

void uacpi_pci_handle_destroy(struct uacpi_pci_handle *const handle) {
    uacpi_kernel_free(handle);
}

uacpi_status
uacpi_kernel_pci_device_open(const uacpi_pci_address address,
                             uacpi_handle *const out_handle)
{
    int flag = 0;
    array_foreach(pci_get_domain_list_locked(&flag),
                  const struct pci_domain *,
                  iter)
    {
        const struct pci_domain *const domain = *iter;
        if (domain->segment != address.segment) {
            continue;
        }

        switch (domain->kind) {
        #if defined(__x86_64__)
            case PCI_DOMAIN_LEGACY:
                if (address.bus != 0) {
                    continue;
                }

                *out_handle = uacpi_pci_handle_create(domain, address);
                pci_release_domain_list_lock(flag);

                if (*out_handle == nullptr) {
                    return UACPI_STATUS_OUT_OF_MEMORY;
                }

                return UACPI_STATUS_OK;
        #endif /* defined(__x86_64__) */

            case PCI_DOMAIN_ECAM: {
                const struct pci_domain_ecam *const ecam =
                    (const struct pci_domain_ecam *)domain;

                if (!range_has_loc(ecam->bus_range, address.bus)) {
                    continue;
                }

                *out_handle = uacpi_pci_handle_create(domain, address);
                pci_release_domain_list_lock(flag);

                if (*out_handle == nullptr) {
                    return UACPI_STATUS_OUT_OF_MEMORY;
                }

                return UACPI_STATUS_OK;
            }
        }

        verify_not_reached();
    }

    pci_release_domain_list_lock(flag);
    return UACPI_STATUS_NOT_FOUND;
}

void uacpi_kernel_pci_device_close(const uacpi_handle handle) {
    uacpi_pci_handle_destroy((struct uacpi_pci_handle *)handle);
}

uacpi_status
uacpi_kernel_pci_read(const uacpi_handle handle,
                      const uacpi_size offset,
                      const uacpi_u8 byte_width,
                      uacpi_u64 *const value)
{
    const struct uacpi_pci_handle *const the_handle =
        (struct uacpi_pci_handle *)handle;

    const uacpi_pci_address address = the_handle->address;
    const struct pci_location location = {
        .segment = address.segment,
        .bus = address.bus,
        .slot = address.device,
        .function = address.function,
    };

    const struct pci_domain *const domain = the_handle->domain;
    switch (byte_width) {
        case sizeof(uint8_t):
            *value = pci_domain_read_8(domain, &location, offset);
            return UACPI_STATUS_OK;
        case sizeof(uint16_t):
            *value = pci_domain_read_16(domain, &location, offset);
            return UACPI_STATUS_OK;
        case sizeof(uint32_t):
            *value = pci_domain_read_32(domain, &location, offset);
            return UACPI_STATUS_OK;
        case sizeof(uint64_t):
            *value = pci_domain_read_64(domain, &location, offset);
            return UACPI_STATUS_OK;
    }

    verify_not_reached();
}

uacpi_status
uacpi_kernel_pci_write(const uacpi_handle device,
                       const uacpi_size offset,
                       const uacpi_u8 byte_width,
                       const uacpi_u64 value)
{
    struct uacpi_pci_handle *const handle = (struct uacpi_pci_handle *)device;

    const struct uacpi_pci_address address = handle->address;
    const struct pci_location location = {
        .segment = address.segment,
        .bus = address.bus,
        .slot = address.device,
        .function = address.function,
    };

    const struct pci_domain *const domain = handle->domain;
    switch (byte_width) {
        case sizeof(uint8_t):
            pci_domain_write_8(domain, &location, offset, value);
            return UACPI_STATUS_OK;
        case sizeof(uint16_t):
            pci_domain_write_16(domain, &location, offset, value);
            return UACPI_STATUS_OK;
        case sizeof(uint32_t):
            pci_domain_write_32(domain, &location, offset, value);
            return UACPI_STATUS_OK;
        case sizeof(uint64_t):
            pci_domain_write_64(domain, &location, offset, value);
            return UACPI_STATUS_OK;
    }

    verify_not_reached();
}

struct pio_range {
    port_t base;
    size_t len;
};

static inline
struct pio_range *pio_range_create(const port_t base, const size_t len) {
    struct pio_range *const range = uacpi_kernel_alloc(sizeof(*range));
    if (range == nullptr) {
        return nullptr;
    }

    range->base = base;
    range->len = len;

    return range;
}

uacpi_status
uacpi_kernel_io_map(const uacpi_io_addr base,
                    const uacpi_size len,
                    uacpi_handle *const out_handle)
{
    struct pio_range *const range = pio_range_create((port_t)base, len);
    if (range == nullptr) {
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    *out_handle = (uacpi_handle)range;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(const uacpi_handle handle) {
    struct pio_range *const range = (struct pio_range *)handle;
    uacpi_kernel_free(range);
}

uacpi_status
uacpi_kernel_io_read(const uacpi_handle handle,
                     const uacpi_size offset,
                     const uacpi_u8 byte_width,
                     uacpi_u64 *const value)
{
    const struct pio_range *const range = (struct pio_range *)handle;
    if (!index_range_in_bounds(RANGE_INIT(offset, byte_width), range->len)) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    *value = pio_read_size(range->base + offset, byte_width);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write(const uacpi_handle handle,
                      const uacpi_size offset,
                      const uacpi_u8 byte_width,
                      const uacpi_u64 value)
{
    const struct pio_range *const range = (struct pio_range *)handle;
    if (!index_range_in_bounds(RANGE_INIT(offset, byte_width), range->len)) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    pio_write_size(range->base + offset, byte_width, value);
    return UACPI_STATUS_OK;
}

static struct list g_vmap_list = LIST_INIT(g_vmap_list);
static struct spinlock g_vmap_lock = SPINLOCK_INIT();

struct uacpi_vmap {
    struct mmio_region *region;
    struct list list;

    struct refcount refcount;
};

void *
create_and_add_vmap(const uacpi_phys_addr addr,
                    const uacpi_size len,
                    const int flag)
{
    struct uacpi_vmap *const vmap = uacpi_kernel_alloc(sizeof(*vmap));
    if (vmap == nullptr) {
        spin_release_restore_intr(&g_vmap_lock, flag);
        return nullptr;
    }

    vmap->region =
        vmap_mmio(RANGE_INIT(addr, len), PROT_READ | PROT_WRITE, /*flags=*/0);

    if (vmap->region == nullptr) {
        spin_release_restore_intr(&g_vmap_lock, flag);
        uacpi_kernel_free(vmap);

        return nullptr;
    }

    list_init(&vmap->list);
    list_add(&g_vmap_list, &vmap->list);

    refcount_init(&vmap->refcount);
    spin_release_restore_intr(&g_vmap_lock, flag);

    return (void *)(uint64_t)vmap->region->base;
}

void *uacpi_kernel_map(const uacpi_phys_addr addr, const uacpi_size len) {
    mm_for_each_memmap(memmap) {
        if (range_has_loc(memmap->range, addr)) {
            return phys_to_virt(addr);
        }
    }

    const uacpi_phys_addr align_addr = align_down(addr, PAGE_SIZE);
    const uacpi_size align_len = align_up_assert(len, PAGE_SIZE);

    const int flag = spin_acquire_save_intr(&g_vmap_lock);
    struct uacpi_vmap *vmap = NULL;

    list_foreach(vmap, &g_vmap_list, list) {
        if (range_has_loc(mmio_region_get_range(vmap->region), align_addr)) {
            ref_up(&vmap->refcount);
            spin_release_restore_intr(&g_vmap_lock, flag);

            return (void *)(uint64_t)vmap->region->base;
        }
    }

    return create_and_add_vmap(align_addr, align_len, flag);
}

void uacpi_kernel_unmap(void *const addr, const uacpi_size len) {
    const struct range range = RANGE_INIT((uacpi_u64)addr, len);
    struct uacpi_vmap *vmap = NULL;

    list_foreach(vmap, &g_vmap_list, list) {
        if (range_has(mmio_region_get_range(vmap->region), range)) {
            if (ref_down(&vmap->refcount)) {
                list_deinit(&vmap->list);
                uacpi_kernel_free(vmap);
            }

            return;
        }
    }
}

static struct simple_alloc g_alloc;
static struct spinlock g_alloc_lock = SPINLOCK_INIT();

#define MAX_SIMPLE_ALLOC_SIZE 512

void *uacpi_kernel_alloc(const uacpi_size size) {
    if (!array_initialized(g_alloc.page_list)) {
        simple_alloc_create(&g_alloc);
    }

    if (size > MAX_SIMPLE_ALLOC_SIZE) {
        return kmalloc(size);
    }

    void *result = NULL;
    with_spinlock_intr_disabled(&g_alloc_lock, {
        result = simple_alloc(&g_alloc, size);
    });

    return result;
}

#ifdef UACPI_NATIVE_ALLOC_ZEROED
/*
 * Allocate a block of memory of 'size' bytes.
 * The returned memory block is expected to be zero-filled.
 */
void *uacpi_kernel_alloc_zeroed(const uacpi_size size) {
    return kmalloc(size);
}
#endif

#ifndef UACPI_SIZED_FREES
void uacpi_kernel_free(void *const mem) {
    if (mem == nullptr) {
        return;
    }

    bool result = false;
    with_spinlock_intr_disabled(&g_alloc_lock, {
        result = simple_try_free(&g_alloc, mem);
    });

    if (!result) {
        kfree(mem);
    }
}
#else
void uacpi_kernel_free(void *const mem, const uacpi_size size_hint) {
    if (mem == nullptr || size_hint == 0) {
        return;
    }

    bool result = false;
    with_spinlock_intr_disabled(&g_alloc_lock, {
        result = simple_try_free(&g_alloc, mem);
    });

    if (!result) {
        kfree(mem);
    }
}
#endif

#ifndef UACPI_FORMATTED_LOGGING
void
uacpi_kernel_log(const uacpi_log_level log_level, const uacpi_char *const str) {
    enum log_level prink_level = LOGLEVEL_INFO;
    switch (log_level) {
        case UACPI_LOG_DEBUG:
            prink_level = LOGLEVEL_DEBUG;
            break;
        case UACPI_LOG_TRACE:
            // FIXME: Maybe add a LOGLEVEL_TRACE
            prink_level = LOGLEVEL_DEBUG;
            break;
        case UACPI_LOG_INFO:
            prink_level = LOGLEVEL_INFO;
            break;
        case UACPI_LOG_WARN:
            prink_level = LOGLEVEL_WARN;
            break;
        case UACPI_LOG_ERROR:
            prink_level = LOGLEVEL_ERROR;
            break;
    }

    printk(prink_level, "%s", str);
}
#else
UACPI_PRINTF_DECL(2, 3)
void
uacpi_kernel_log(const uacpi_log_level log_level,
                 const uacpi_char *const str,
                 ...)
{
    va_list list;

    va_start(list, str);
    uacpi_kernel_vlog(log_level, str, list);
    va_end(list);
}

void
uacpi_kernel_vlog(const uacpi_log_level log_level,
                  const uacpi_char *const str,
                  va_list list)
{
    enum log_level prink_level = LOGLEVEL_INFO;
    switch (log_level) {
        case UACPI_LOG_DEBUG:
            prink_level = LOGLEVEL_DEBUG;
            break;
        case UACPI_LOG_TRACE:
            // FIXME: Maybe add a LOGLEVEL_TRACE
            prink_level = LOGLEVEL_DEBUG;
            break;
        case UACPI_LOG_INFO:
            prink_level = LOGLEVEL_INFO;
            break;
        case UACPI_LOG_WARN:
            prink_level = LOGLEVEL_WARN;
            break;
        case UACPI_LOG_ERROR:
            prink_level = LOGLEVEL_ERROR;
            break;
    }

    vprintk(prink_level, str, list);
}
#endif

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return (uacpi_u64)nsec_since_boot();
}

void uacpi_kernel_stall(const uacpi_u8 usec) {
    stall_for_usec(usec);
}

void uacpi_kernel_sleep(const uacpi_u64 usec) {
    sched_sleep_us(usec);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    struct mutex *const result = uacpi_kernel_alloc(sizeof(*result));
    if (result == nullptr) {
        return UACPI_NULL;
    }

    mutex_init(result);
    return result;
}

void uacpi_kernel_free_mutex(const uacpi_handle handle) {
    struct mutex *const mutex = (struct mutex *)handle;
    uacpi_kernel_free(mutex);
}

struct uacpi_event {
    _Atomic(uint64_t) counter;
};

bool uacpi_event_try_decrement(struct uacpi_event *const event) {
    while (true) {
        uint64_t counter =
            atomic_load_explicit(&event->counter, memory_order_acquire);

        if (counter == 0) {
            return false;
        }

        const bool success =
            atomic_compare_exchange_strong_explicit(&event->counter,
                                                    &counter,
                                                    counter - 1,
                                                    memory_order_acq_rel,
                                                    memory_order_acquire);

        if (success) {
            return true;
        }
    };

    return true;
}

uacpi_handle uacpi_kernel_create_event(void) {
    struct uacpi_event *const result = uacpi_kernel_alloc(sizeof(*result));
    if (result == nullptr) {
        return UACPI_NULL;
    }

    result->counter = 0;
    return result;
}

void uacpi_kernel_free_event(const uacpi_handle handle) {
    struct uacpi_event *const event = handle;
    uacpi_kernel_free(event);
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return current_thread();
}

uacpi_status
uacpi_kernel_acquire_mutex(const uacpi_handle handle, const uacpi_u16 timeout) {
    (void)timeout;

    struct mutex *const lock = (struct mutex *)handle;
    switch (timeout) {
        case 0:
            if (mutex_try_lock(lock)) {
                return UACPI_STATUS_OK;
            }

            return UACPI_STATUS_TIMEOUT;
        case 1 ... 0xFFFF:
            if (mutex_lock_with_timeout(lock, timeout)) {
                return UACPI_STATUS_OK;
            }

            return UACPI_STATUS_TIMEOUT;
    }

    mutex_lock(lock);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(const uacpi_handle handle) {
    struct mutex *const lock = (struct mutex *)handle;
    mutex_unlock(lock);
}

#define UACPI_SLEEP_INCREMENT_USEC (usec_t)15

uacpi_bool
uacpi_kernel_wait_for_event(const uacpi_handle handle, const uacpi_u16 timeout)
{
    struct uacpi_event *const event = (struct uacpi_event *)handle;
    while (true) {
        if (uacpi_event_try_decrement(event)) {
            return UACPI_TRUE;
        }

        if (timeout == 0xFFFF) {
            continue;
        }

        if (timeout == 0) {
            return UACPI_FALSE;
        }

        const usec_t wait_time = min(UACPI_SLEEP_INCREMENT_USEC, timeout);
        uacpi_kernel_sleep(wait_time);
    }

    verify_not_reached();
}

void uacpi_kernel_signal_event(const uacpi_handle handle) {
    struct uacpi_event *const event = (struct uacpi_event *)handle;
    atomic_fetch_add_explicit(&event->counter, 1, memory_order_acq_rel);
}

void uacpi_kernel_reset_event(const uacpi_handle handle) {
    struct uacpi_event *const event = (struct uacpi_event *)handle;
    atomic_store_explicit(&event->counter, 0, memory_order_release);
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request *const request) {
    switch (request->type) {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            printk(LOGLEVEL_INFO, "uacpi: got breakpoint. ignoring\n");
            return UACPI_STATUS_OK;
        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            printk(LOGLEVEL_CRITICAL,
                   "uacpi: got fatal firmware error:\n"
                   "\ttype: 0x%" PRIx8 "\n"
                   "\tcode: 0x%" PRIx32 "\n"
                   "\targ: 0x%" PRIx64 "\n",
                   request->fatal.type,
                   request->fatal.code,
                   request->fatal.arg);
            return UACPI_STATUS_OK;
    }

    verify_not_reached();
}

struct uacpi_irq_context {
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
};

struct uacpi_irq_context *
uacpi_irq_context_create(const uacpi_handle handler, const uacpi_handle ctx) {
    struct uacpi_irq_context *const result =
        uacpi_kernel_alloc(sizeof(*result));

    if (result == nullptr) {
        return UACPI_NULL;
    }

    result->handler = handler;
    result->ctx = ctx;

    return result;
}

static void
uacpi_irq_handler(const uint64_t intr_no,
                  struct thread_context *const frame,
                  void *const ctx)
{
    (void)intr_no;
    (void)frame;

    struct uacpi_irq_context *const irq_ctx = (struct uacpi_irq_context *)ctx;
    irq_ctx->handler(irq_ctx->ctx);

    isr_eoi(intr_no);
}

uacpi_status
uacpi_kernel_install_interrupt_handler(const uacpi_u32 irq,
                                       const uacpi_interrupt_handler handler,
                                       const uacpi_handle ctx,
                                       uacpi_handle *const out_irq_handle)
{
    struct uacpi_irq_context *const context =
        uacpi_irq_context_create(handler, ctx);

    if (context == nullptr) {
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    struct irq_pin *const pin = isr_get_irq_pin(irq);
    if (isr_install_irq(pin, uacpi_irq_handler, context, /*masked=*/false)) {
        *out_irq_handle = (void *)(uintptr_t)irq;
        return UACPI_STATUS_OK;
    }

    // FIXME: Return the proper error
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(
    const uacpi_interrupt_handler handler,
    const uacpi_handle irq_handle)
{
    (void)handler;
    struct irq_pin *const pin =
        isr_get_irq_pin((uacpi_u32)(uintptr_t)irq_handle);

    uacpi_kernel_free(isr_uninstall_irq(pin));
    return UACPI_STATUS_OK;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    struct spinlock *const lock = uacpi_kernel_alloc(sizeof(*lock));
    if (lock == nullptr) {
        return UACPI_NULL;
    }

    *lock = SPINLOCK_INIT();
    return lock;
}

void uacpi_kernel_free_spinlock(const uacpi_handle handle) {
    struct spinlock *const spinlock = (struct spinlock *)handle;
    uacpi_kernel_free(spinlock);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(const uacpi_handle handle) {
    struct spinlock *const lock = (struct spinlock *)handle;
    return (uacpi_cpu_flags)spin_acquire_save_intr(lock);
}

void
uacpi_kernel_unlock_spinlock(const uacpi_handle handle,
                             const uacpi_cpu_flags flags)
{
    struct spinlock *const lock = (struct spinlock *)handle;
    spin_release_restore_intr(lock, flags);
}

struct uacpi_work {
    uacpi_work_handler handler;
    uacpi_handle ctx;
    uacpi_handle work;
};

uacpi_status
uacpi_kernel_schedule_work(const uacpi_work_type work_type,
                           const uacpi_work_handler handler,
                           const uacpi_handle ctx)
{
    (void)work_type;
    (void)handler;
    (void)ctx;

    // TODO:
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    // TODO:
    return UACPI_STATUS_OK;
}
