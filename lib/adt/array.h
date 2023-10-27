/*
 * lib/adt/array.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

#include "growable_buffer.h"
#include "range.h"

struct array {
    struct growable_buffer gbuffer;
    uint32_t object_size;
};

#define array_foreach(array, type, item) \
    assert(sizeof(type) == (array)->object_size);                              \
    type *const VAR_CONCAT(__begin__, __LINE__) =                              \
        (type *)array_begin(*(array));                                         \
    const type *const VAR_CONCAT(__end__, __LINE__) =                          \
        VAR_CONCAT(__begin__, __LINE__) + array_item_count(*(array));          \
    for (type *item = VAR_CONCAT(__begin__, __LINE__);                         \
         item != VAR_CONCAT(__end__, __LINE__);                                \
         item++)

#define ARRAY_INIT(size) \
    ((struct array){ .gbuffer = GBUFFER_INIT(), .object_size = (size)})

void array_init(struct array *array, uint32_t object_size);
struct array array_alloc(uint32_t object_size, uint32_t item_capacity);

bool array_append(struct array *array, const void *item);
void array_remove_index(struct array *array, uint32_t index);
bool array_remove_range(struct array *array, struct range range);

void *array_begin(struct array array);
const void *array_end(struct array array);

void *array_at(struct array array, uint64_t index);

void *array_front(struct array array);
void *array_back(struct array array);

uint64_t array_item_count(struct array array);
uint64_t array_free_count(struct array array);

void *array_take_data(struct array *array);
void array_take_item(struct array *array, uint32_t index, void *item);
void array_take_range(struct array *array, struct range range, void *item);

bool array_empty(struct array array);
void array_destroy(struct array *array);
