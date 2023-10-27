/*
 * lib/list.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "macros.h"

// list is a circular doubly-linked list
struct list {
    struct list *prev;
    struct list *next;
};

#define LIST_INIT(name) { .prev = &(name), .next = &(name) }

__optimize(3) static inline void list_init(struct list *const head) {
    head->prev = head;
    head->next = head;
}

__optimize(3) static inline void
list_add_common(struct list *const elem,
                struct list *const prev,
                struct list *const next)
{
    elem->prev = prev;
    elem->next = next;

    prev->next = elem;
    next->prev = elem;
}

// Add to front of list
__optimize(3)
static inline void list_add(struct list *const head, struct list *const item) {
    list_add_common(item, head, head->next);
}

// Add to back of list
__optimize(3)
static inline void list_radd(struct list *const head, struct list *const item) {
    list_add_common(item, head->prev, head);
}

__optimize(3) static inline bool list_empty(struct list *const list) {
    return list == list->prev;
}

__optimize(3) static inline void list_remove(struct list *const elem) {
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;

    elem->prev = elem;
    elem->next = elem;
}

__optimize(3) static inline void list_delete(struct list *const elem) {
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;

    elem->prev = NULL;
    elem->next = NULL;
}

#define list_rm(type, elem, name) \
    ({ list_remove(elem); container_of(elem, type, name); })

#define list_del(type, elem, name) \
    ({ list_delete(elem); container_of(elem, type, name); })

#define list_prev(ob, name) container_of(ob->name.prev, typeof(*(ob)), name)
#define list_next(ob, name) container_of(ob->name.next, typeof(*(ob)), name)

#define list_prev_safe(ob, name, list) \
    (ob->name.prev != (list) ? list_prev(ob, name) : NULL)

#define list_next_safe(ob, name, list) \
    (ob->name.next != (list) ? list_next(ob, name) : NULL)

#define list_head(list, type, name) \
    ((type *)((void *)((char *)(list)->next - offsetof(type, name))))
#define list_tail(list, type, name) \
    ((type *)((void *)((char *)(list)->prev - offsetof(type, name))))

#define list_foreach(iter, list, name) \
    for(iter = list_head(list, typeof(*iter), name); &iter->name != (list); \
        iter = list_next(iter, name))

#define list_foreach_reverse(iter, list, name) \
    for(iter = list_tail(list, typeof(*iter), name); &iter->name != (list); \
        iter = list_prev(iter, name))

#define list_count(list, type, name) ({  \
    uint64_t __result__ = 0;             \
    type *__iter__ = NULL;               \
    list_foreach(__iter__, list, name) { \
        __result__++;                    \
    }                                    \
    __result__;                          \
})

#define list_foreach_mut(iter, tmp, list, name) \
    for (iter = list_head(list, typeof(*iter), name), \
             tmp = list_next(iter, name);             \
         &iter->name != (list);                       \
         iter = tmp, tmp = list_next(iter, name))

#define list_foreach_reverse_mut(iter, tmp, list, name) \
    for (iter = list_tail(list, typeof(*iter), name), \
             tmp = list_prev(iter, name);             \
         &iter->name != (list);                       \
         iter = tmp, tmp = list_prev(iter, name))
