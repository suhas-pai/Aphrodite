/*
 * lib/adt/growable_buffer.c
 * © suhas pai
 */

#include "lib/alloc.h"
#include "lib/util.h"

#include "growable_buffer.h"

__optimize(3) struct growable_buffer gbuffer_alloc(const uint32_t capacity) {
    uint32_t size = 0;
    const struct growable_buffer gbuffer = {
        .begin = malloc_size(capacity, &size),
        .capacity = size,
        .is_alloc = true
    };

    return gbuffer;
}

__optimize(3) struct growable_buffer
gbuffer_alloc_copy(void *const data, const uint32_t size) {
    const struct growable_buffer gbuffer = gbuffer_alloc(size);
    if (gbuffer.begin != NULL) {
        memcpy(gbuffer.begin, data, size);
    }

    return gbuffer;
}

__optimize(3)
struct growable_buffer gbuffer_copy(const struct growable_buffer gbuffer) {
    struct growable_buffer result = GBUFFER_INIT();
    if (gbuffer_used_size(gbuffer) == 0) {
        return result;
    }

    result = gbuffer_alloc(gbuffer_used_size(gbuffer));
    if (result.begin != NULL) {
        memcpy(result.begin, gbuffer.begin, gbuffer_used_size(gbuffer));
        result.index = gbuffer.index;
    }

    return result;
}

__optimize(3) struct growable_buffer
gbuffer_open(void *const buffer,
             const uint32_t used,
             const uint32_t capacity,
             const bool is_alloc)
{
    const struct growable_buffer gbuffer = {
        .begin = buffer,
        .index = used,
        .capacity = capacity,
        .is_alloc = is_alloc
    };

    return gbuffer;
}

__optimize(3) struct growable_buffer
gbuffer_open_mbuffer(const struct mutable_buffer mbuffer, const bool is_alloc) {
    const struct growable_buffer gbuffer =
        gbuffer_open(mbuffer.begin,
                     mbuffer_used_size(mbuffer),
                     mbuffer_capacity(mbuffer),
                     is_alloc);
    return gbuffer;
}

bool
gbuffer_ensure_can_add_capacity(struct growable_buffer *const gb, uint32_t add)
{
    if (gbuffer_free_space(*gb) >= add) {
        return true;
    }

    // Avoid using realloc() here because by using malloc_size() we know that
    // we've used up all memory that was available to us.

    uint32_t new_size = check_add_assert((uint32_t)gb->capacity, add);
    void *const new_alloc = malloc_size(new_size, &new_size);

    if (new_alloc == NULL) {
        return false;
    }

    memcpy(new_alloc, gb->begin, gbuffer_used_size(*gb));
    if (gb->is_alloc) {
        free(gb->begin);
    } else {
        gb->is_alloc = true;
    }

    gb->begin = new_alloc;
    gb->capacity = new_size;

    return true;
}

__optimize(3) struct mutable_buffer
gbuffer_get_mutable_buffer(const struct growable_buffer gbuffer) {
    return mbuffer_open(gbuffer.begin, gbuffer.index, gbuffer.capacity);
}

__optimize(3) void *gbuffer_current_ptr(const struct growable_buffer gbuffer) {
    return gbuffer.begin + gbuffer.index;
}

__optimize(3) void *
gbuffer_at(const struct growable_buffer gbuffer, const uint32_t byte_index) {
    void *const result = gbuffer.begin + byte_index;
    const uint32_t capacity = gbuffer.capacity;

    assert_msg(index_in_bounds(byte_index, capacity),
               "gbuffer: attempting to access past end of buffer, at "
               "byte-index: %" PRIu32 ", end: %" PRIu32 "\n",
               byte_index,
               capacity);

    return result;
}

__optimize(3) const void *gbuffer_end(const struct growable_buffer gbuffer) {
    return gbuffer.begin + gbuffer.capacity;
}

__optimize(3)
uint32_t gbuffer_free_space(const struct growable_buffer gbuffer) {
    return gbuffer.capacity - gbuffer.index;
}

__optimize(3) uint32_t gbuffer_used_size(const struct growable_buffer gbuffer) {
    return gbuffer.index;
}

__optimize(3) bool gbuffer_empty(const struct growable_buffer gbuffer) {
    return gbuffer.index == 0;
}

__optimize(3) uint32_t
gbuffer_increment_ptr(struct growable_buffer *const gbuffer,
                      const uint32_t amount)
{
    const uint32_t delta = min(gbuffer_free_space(*gbuffer), amount);
    gbuffer->index = delta;

    return delta;
}

__optimize(3) uint32_t
gbuffer_decrement_ptr(struct growable_buffer *const gbuffer, const uint32_t amt)
{
    const uint32_t delta = min((uint32_t)gbuffer->index, amt);
    gbuffer->index -= delta;

    return delta;
}

__optimize(3) bool
gbuffer_append_data(struct growable_buffer *const gbuffer,
                    const void *const data,
                    const uint32_t length)
{
    if (!gbuffer_ensure_can_add_capacity(gbuffer, length)) {
        return false;
    }

    memcpy(gbuffer_current_ptr(*gbuffer), data, length);
    gbuffer->index += length;

    return true;
}

__optimize(3) bool
gbuffer_append_byte(struct growable_buffer *const gbuffer,
                    const uint8_t byte,
                    const uint32_t count)
{
    if (!gbuffer_ensure_can_add_capacity(gbuffer, count)) {
        return false;
    }

    memset(gbuffer_current_ptr(*gbuffer), byte, count);
    gbuffer->index += count;

    return true;
}

__optimize(3) bool
gbuffer_append_gbuffer_data(struct growable_buffer *const gbuffer,
                            const struct growable_buffer *const append)
{
    return gbuffer_append_data(gbuffer, append->begin, append->index);
}

__optimize(3) uint32_t
gbuffer_append_sv(struct growable_buffer *const gbuffer,
                  const struct string_view sv)
{
    return gbuffer_append_data(gbuffer, sv.begin, sv.length);
}

__optimize(3) void
gbuffer_remove_index(struct growable_buffer *const gbuffer,
                     const uint32_t index)
{
    const uint32_t used = gbuffer->index;
    assert(index_in_bounds(index, used));

    if (index != used - 1) {
        void *const dst = gbuffer->begin + index;
        memmove(dst, dst + 1, used - index);
    }

    gbuffer->index = used - 1;
}

__optimize(3) void
gbuffer_remove_range(struct growable_buffer *const gbuffer,
                     const struct range range)
{
    const uint32_t used = gbuffer->index;
    assert(index_range_in_bounds(range, used));

    const uint32_t end = range_get_end_assert(range);
    if (end != used) {
        memmove(gbuffer->begin + range.front, gbuffer->begin + end, used - end);
    }

    gbuffer->index = used - range.size;
}

__optimize(3) void
gbuffer_truncate(struct growable_buffer *const gbuffer,
                 const uint32_t byte_index)
{
    gbuffer->index = min((uint32_t)gbuffer->index, byte_index);
}

__optimize(3) void *gbuffer_take_data(struct growable_buffer *const gbuffer) {
    void *const result = gbuffer->begin;

    gbuffer->begin = NULL;
    gbuffer->index = 0;
    gbuffer->capacity = 0;
    gbuffer->is_alloc = false;

    return result;
}

__optimize(3) void gbuffer_clear(struct growable_buffer *const gbuffer) {
    gbuffer->index = 0;
}

__optimize(3) void gbuffer_destroy(struct growable_buffer *const gb) {
    if (gb->is_alloc) {
        free(gb->begin);
    } else {
        // Even if just a stack buffer, clear out memory for safety
        bzero(gb->begin, gb->capacity);
    }

    gb->begin = NULL;
    gb->index = 0;
    gb->capacity = 0;
    gb->is_alloc = false;
}