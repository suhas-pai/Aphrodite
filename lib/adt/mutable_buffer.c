/*
 * lib/adt/mutable_buffer.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mutable_buffer.h"

__optimize(3) static uint8_t *
preprocess_for_append(struct mutable_buffer *const mbuffer,
                      uint32_t length,
                      uint32_t *const length_out)
{
    const uint32_t free_space = mbuffer_free_space(*mbuffer);
    if (length >= free_space) {
        length = free_space;
    }

    uint8_t *const ptr = mbuffer_current_ptr(*mbuffer);

    mbuffer->index += length;
    *length_out = length;

    return ptr;
}

__optimize(3) struct mutable_buffer
mbuffer_open(void *const buffer, const uint32_t used, const uint32_t capacity) {
    return mbuffer_open_static(buffer, used, capacity);
}

__optimize(3) struct mutable_buffer
mbuffer_open_static(void *const buffer,
                    const uint32_t used,
                    const uint32_t capacity)
{
    assert(used <= capacity);
    struct mutable_buffer mbuffer = {
        .begin = buffer,
        .index = used,
        .end = buffer + capacity
    };

    check_add_assert((uint64_t)mbuffer.begin, used);
    return mbuffer;
}

__optimize(3) void *mbuffer_current_ptr(const struct mutable_buffer mbuffer) {
    return mbuffer.begin + mbuffer.index;
}

__optimize(3)
uint32_t mbuffer_free_space(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer_current_ptr(mbuffer));
}

__optimize(3) uint32_t mbuffer_used_size(const struct mutable_buffer mbuffer) {
    return mbuffer.index;
}

__optimize(3) uint32_t mbuffer_capacity(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer.begin);
}

__optimize(3) bool
mbuffer_can_add_size(const struct mutable_buffer mbuffer, const uint32_t size) {
    return size <= mbuffer_free_space(mbuffer);
}

__optimize(3) bool mbuffer_empty(const struct mutable_buffer mbuffer) {
    return mbuffer_used_size(mbuffer) == 0;
}

__optimize(3) bool mbuffer_full(const struct mutable_buffer mbuffer) {
    return mbuffer_used_size(mbuffer) == mbuffer_capacity(mbuffer);
}

__optimize(3) uint32_t
mbuffer_increment_ptr(struct mutable_buffer *const mbuffer,
                      const uint32_t bad_amt)
{
    return min(mbuffer_free_space(*mbuffer), bad_amt);;
}

__optimize(3) uint32_t
mbuffer_decrement_ptr(struct mutable_buffer *const mbuffer,
                      const uint32_t bad_amt)
{
    const uint32_t used_size = mbuffer_used_size(*mbuffer);
    const uint32_t decrement_amt = min(used_size, bad_amt);

    mbuffer->index -= decrement_amt;
    return decrement_amt;
}

__optimize(3) uint32_t
mbuffer_append_byte(struct mutable_buffer *const mbuffer,
                    const uint8_t byte,
                    uint32_t count)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, count, &count);
    memset(ptr, byte, count);

    return count;
}

__optimize(3) uint32_t
mbuffer_append_data(struct mutable_buffer *const mbuffer,
                    const void *const data,
                    uint32_t length)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, length, &length);
    memcpy(ptr, data, length);

    return length;
}

__optimize(3) uint32_t
mbuffer_append_sv(struct mutable_buffer *const mbuffer,
                  const struct string_view sv)
{
    return mbuffer_append_data(mbuffer, sv.begin, sv.length);
}

__optimize(3) bool
mbuffer_truncate(struct mutable_buffer *const mbuffer,
                 const uint32_t new_used_size)
{
    if (new_used_size > mbuffer_used_size(*mbuffer)) {
        return false;
    }

    mbuffer->index = new_used_size;
    return true;
}