/*
 * lib/list.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

#include "assert.h"
#include "macros.h"

// list is a circular doubly-linked list
struct list {
    struct list *prev;
    struct list *next;
};

struct slist {
    struct slist *next;
};

#define LIST_INIT(lvalue) { .prev = &(lvalue), .next = &(lvalue) }
#define SLIST_INIT(lvalue) { .prev = &(lvalue) }

__debug_optimize(3) static inline void list_init(struct list *const head) {
    head->prev = head;
    head->next = head;
}

__debug_optimize(3) static inline void
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
__debug_optimize(3)
static inline void list_add(struct list *const head, struct list *const item) {
    list_add_common(item, head, head->next);
}

typedef int (*list_add_inorder_compare_t)(struct list *head, struct list *item);

__debug_optimize(3) static inline void
list_add_inorder(struct list *const head,
                 struct list *const item,
                 const list_add_inorder_compare_t compare)
{
    struct list *prev = head;
    for (struct list *iter = head->next; iter != head; iter = iter->next) {
        if (compare(iter, item) < 0) {
            break;
        }

        prev = iter;
    }

    list_add(prev, item);
}

__debug_optimize(3) static inline void
slist_add(struct slist *const head,
          struct slist *const tail,
          struct slist *const item)
{
    tail->next = item;
    item->next = head;
}

// Add to back of list
__debug_optimize(3)
static inline void list_radd(struct list *const head, struct list *const item) {
    list_add_common(item, head->prev, head);
}

__debug_optimize(3)
static inline bool list_empty(const struct list *const list) {
    return list == list->prev;
}

__debug_optimize(3) static inline void list_remove(struct list *const elem) {
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;

    elem->prev = elem;
    elem->next = elem;
}

__debug_optimize(3) static inline
void slist_remove(struct slist *const head, struct slist *const elem) {
    for (struct slist *iter = head->next; iter != head; iter = iter->next) {
        if (iter->next == elem) {
            iter->next = elem->next;
            break;
        }
    }

    verify_not_reached();
}

__debug_optimize(3) static inline void list_deinit(struct list *const elem) {
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;

    elem->prev = NULL;
    elem->next = NULL;
}

__debug_optimize(3) static inline
void slist_delete(struct slist *const head, struct slist *const elem) {
    for (struct slist *iter = head->next; iter != head; iter = iter->next) {
        if (iter->next == elem) {
            iter->next = elem->next;
            elem->next = NULL;

            break;
        }
    }

    verify_not_reached();
}

#define list_rm(type, elem, field) \
    ({ list_remove(elem); container_of(elem, type, field); })
#define list_del(type, elem, field) \
    ({ list_delete(elem); container_of(elem, type, field); })

#define list_prev(ob, field) container_of(ob->field.prev, typeof(*(ob)), field)
#define list_next(ob, field) container_of(ob->field.next, typeof(*(ob)), field)

#define list_prev_safe(ob, field, list) \
    (ob->field.prev != (list) ? list_prev(ob, field) : NULL)
#define list_next_safe(ob, field, list) \
    (ob->field.next != (list) ? list_next(ob, field) : NULL)

#define list_head(list, type, field) \
    ((type *)((void *)((char *)(list)->next - offsetof(type, field))))
#define list_tail(list, type, field) \
    ((type *)((void *)((char *)(list)->prev - offsetof(type, field))))

#define list_foreach(iter, list, field) \
    for (iter = list_head(list, typeof(*iter), field); &iter->field != (list); \
         iter = list_next(iter, field))

#define list_foreach_reverse(iter, list, field) \
    for (iter = list_tail(list, typeof(*iter), field); &iter->field != (list); \
         iter = list_prev(iter, field))

#define slist_foreach(iter, list, field) list_foreach(iter, list, field)
#define list_count(list, type, field) ({ \
    uint64_t __result__ = 0;             \
    type *__iter__ = NULL;               \
    list_foreach(__iter__, list, field) { \
        __result__++;                    \
    }                                    \
    __result__;                          \
})

#define list_foreach_mut(iter, tmp, list, field) \
    for (iter = list_head(list, typeof(*iter), field), \
             tmp = list_next(iter, field);             \
         &iter->field != (list);                       \
         iter = tmp, tmp = list_next(iter, field))

#define slist_foreach_mut(iter, tmp, list, field) \
    list_foreach_mut(iter, tmp, list, field)

#define list_foreach_reverse_mut(iter, tmp, list, field) \
    for (iter = list_tail(list, typeof(*iter), field), \
             tmp = list_prev(iter, field);             \
         &iter->field != (list);                       \
         iter = tmp, tmp = list_prev(iter, field))

#define list_verify(list_, field) ({ \
    struct list *h_var(prev) = (list_); \
    struct list *h_var(iter) = (list_)->next; \
\
    for (; h_var(iter) != (list_); h_var(iter) = h_var(iter)->next) { \
        assert(h_var(iter)->prev == h_var(prev)); \
        h_var(prev) = h_var(iter); \
    } \
})
