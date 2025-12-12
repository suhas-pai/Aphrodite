/*
 * lib/adt/array.h
 * © suhas pai
 */

#pragma once

#include "growable_buffer.h"
#include "range.h"

struct array {
    struct growable_buffer gbuffer;
    uint32_t object_size;
};

#define array_foreach(list, type, item) \
    struct array h_var(array) = *(list);                                       \
    assert(sizeof(type) == h_var(array).object_size);                          \
                                                                               \
    ptrrange_foreach((type *)array_begin(h_var(array)),                        \
                     cast_to_ptr(type, array_end(h_var(array))),               \
                     item)

#define array_foreach_mut(list, type, item) \
    struct array h_var(array) = *(list);                                       \
    assert(sizeof(type) == h_var(array).object_size);                          \
                                                                               \
    ptrrange_foreach_mut((type *)array_begin(h_var(array)),                    \
                         cast_to_ptr(type, array_end(h_var(array))),           \
                         item)

#define array_foreach_from_index(list, type, item, index) \
    struct array h_var(array) = *(list);                                       \
                                                                               \
    assert(sizeof(type) == h_var(array)->object_size);                         \
    assert(index_in_bounds(index, array_item_count(h_var(array))));            \
                                                                               \
    ptrrange_foreach((type *)array_begin(h_var(array)) + index,                \
                     cast_to_ptr(type, array_end(h_var(array))),               \
                     item)

#define array_foreach_mut_from_index(list, type, item, index) \
    struct array h_var(array) = *(list);                                       \
                                                                               \
    assert(sizeof(type) == h_var(array)->object_size);                         \
    assert(index_in_bounds(index, array_item_count(h_var(array))));            \
                                                                               \
    ptrrange_foreach_mut((type *)array_begin(h_var(array)) + index,            \
                         cast_to_ptr(type, array_end(h_var(array))),           \
                         item)

#define ARRAY_INIT(size) \
    ((struct array){ .gbuffer = GBUFFER_INIT(), .object_size = (size) })

#define ARRAY_ONE_ITEM(item) \
    ((struct array){ \
        .gbuffer = GBUFFER_WITH_ITEM(item, sizeof(item)), \
        .object_size = sizeof(item) \
    })

struct array *array_alloc(uint32_t object_size, uint32_t item_capacity);
struct array array_copy(struct array array);

bool array_initialized(struct array array);
bool array_add(struct array *array, const void *item);

void array_remove_index(struct array *array, uint32_t index);
void array_remove_item(struct array *array, const void *item);

bool array_remove_range(struct array *array, struct range range);

void *array_begin(struct array array);
const void *array_end(struct array array);

void *array_get_at(struct array array, uint32_t index);

void *array_get_front(struct array array);
void *array_get_back(struct array array);

#define array_at(the_array, type, index) ({ \
    const auto h_var(array) = (the_array); \
    assert(h_var(array).object_size == sizeof(type)); \
    ((type *)array_get_at(h_var(array), index)); \
})

#define array_front(the_array, type) ({ \
    const auto h_var(array) = (the_array); \
    assert(h_var(array).object_size == sizeof(type)); \
    ((type *)array_get_front(h_var(array))); \
})

#define array_back(array, type) ({ \
    assert((array)->object_size == sizeof(type)); \
    ((type *)array_get_back(*(array))); \
})

uint32_t array_indexof(struct array array, const void *item);

uint32_t array_item_count(struct array array);
uint32_t array_free_count(struct array array);

void *array_take_data(struct array *array);
void array_take_item(struct array *array, uint32_t index, void *item);
void array_take_range(struct array *array, struct range range, void *item);

void array_reserve(struct array *array, uint32_t amount);
void array_reserve_and_set_item_count(struct array *array, uint32_t amount);

bool array_empty(struct array array);
void array_destroy(struct array *array);

void array_free(struct array *array);
