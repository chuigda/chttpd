#include "cc_list.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CC_LIST CCTY(cc_list)
#define CC_LIST_ITER CCTY(cc_list_iter)

typedef struct st_list_node {
    struct st_list_node *prev;
    struct st_list_node *next;
    uint8_t data[0];
} list_node;

void
CCFN(cc_list_init) (CC_LIST *list, size_t elem_size) {
    list_node *head_node = (list_node*)list;
    head_node->prev = head_node;
    head_node->next = head_node;
    
    list->elem_size = elem_size;
}

void
CCFN(cc_list_destroy) (CC_LIST *list) {
    list_node *head_node = (list_node*)list;
    list_node *iter_node = head_node->next;
    while (iter_node != head_node) {
        list_node *t = iter_node;
        iter_node = iter_node->next;
        free (t);
    }
}

void
CCFN(cc_list_push_back) (CC_LIST *list, const void *data) {
    list_node *new_node = 
        (list_node*)malloc(sizeof(list_node) + list->elem_size);
    memcpy (CC_VPTR_ADD(new_node, CC_OFFSETOF(list_node, data)),
            data,
            list->elem_size);
    
    list_node *head_node = (list_node*)list;
    new_node->prev = head_node->prev;
    new_node->next = head_node;
    head_node->prev->next = new_node;
    head_node->prev = new_node;
}

void
CCFN(cc_list_push_front) (CC_LIST *list, const void *data) {
    list_node *new_node = 
        (list_node*)malloc(sizeof(list_node) + list->elem_size);
    memcpy (CC_VPTR_ADD(new_node, CC_OFFSETOF(list_node, data)),
            data,
            list->elem_size);
    
    list_node *head_node = (list_node*)list;
    new_node->prev = head_node;
    new_node->next = head_node->next;
    head_node->next->prev = new_node;
    head_node->next = new_node;
}

void
CCFN(cc_list_pop_back) (CC_LIST *list) {
    assert(!CCFN(cc_list_empty)(list));
    
    list_node *head_node = (list_node*)list;
    list_node *back_node = head_node->prev;
    back_node->prev->next = back_node->next;
    back_node->next->prev = back_node->prev;
    free(back_node);
}

void
CCFN(cc_list_pop_front) (CC_LIST *list) {
    assert(!CCFN(cc_list_empty)(list));
    
    list_node *head_node = (list_node*)list;
    list_node *front_node = head_node->next;
    front_node->next->prev = front_node->prev;
    front_node->prev->next = front_node->next;
    free(front_node);
}

void*
CCFN(cc_list_front) (const CC_LIST *list) {
    assert(!CCFN(cc_list_empty)(list));

    list_node *head_node = (list_node*)list;
    list_node *front_node = head_node->next;
    return CC_VPTR_ADD(front_node, CC_OFFSETOF(list_node, data));
}

void*
CCFN(cc_list_back) (const CC_LIST *list) {
    assert(!CCFN(cc_list_empty)(list));
    
    list_node *head_node = (list_node*)list;
    list_node *back_node = head_node->prev;
    return CC_VPTR_ADD(back_node, CC_OFFSETOF(list_node, data));
}

size_t
CCFN(cc_list_len) (const CC_LIST *list) {
    return CCFN(cc_list_size) (list);
}

size_t
CCFN(cc_list_size) (const CC_LIST *list) {
    size_t ret = 0;
    list_node *head_node = (list_node*)list;
    list_node *iter_node = head_node->next;
    while (iter_node != head_node) {
        iter_node = iter_node->next;
        ret++;
    }
    return ret;
}

_Bool
CCFN(cc_list_empty) (const CC_LIST *list) {
    list_node *head_node = (list_node*)list;
    return head_node->next == head_node;
}

CC_LIST_ITER
CCFN(cc_list_begin) (const CC_LIST *list) {
    CC_LIST_ITER ret;
    list_node *head_node = (list_node*)list;
    ret.node = head_node->next;
    return ret;
}

CC_LIST_ITER
CCFN(cc_list_end) (const CC_LIST *list) {
    CC_LIST_ITER ret;
    ret.node = (CC_LIST*)list;
    return ret;
}

void
CCFN(cc_list_insert) (const CC_LIST *list,
                      CC_LIST_ITER pos,
                      const void *data) {
    list_node *new_node = 
        (list_node*)malloc(sizeof(list_node) + list->elem_size);
    memcpy (CC_VPTR_ADD(new_node, CC_OFFSETOF(list_node, data)),
            data,
            list->elem_size);
    list_node *node = (list_node*)pos.node;
    new_node->prev = node->prev;
    new_node->next = node;
    node->prev->next = new_node;
    node->prev = new_node;
}

void
CCFN(cc_list_insert_after) (const CC_LIST *list,
                            CC_LIST_ITER pos,
                            const void *data) {
    CCFN(cc_list_insert) (list,
                          CCFN(cc_list_iter_next) (pos),
                          data);
}

void
CCFN(cc_list_remove) (const CC_LIST *list,
                      CC_LIST_ITER pos) {
    (void)list;
    list_node *node = (list_node*)pos.node;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free (node);
}

void
CCFN(cc_list_remove_range) (const CC_LIST *list,
                            CC_LIST_ITER first,
                            CC_LIST_ITER last) {
    (void)list;
    list_node *first_node = (list_node*)first.node;
    list_node *last_node = (list_node*)last.node;
    first_node->prev->next = last_node;
    last_node->prev = first_node->prev;
    
    while (first_node != last_node) {
        list_node *t = first_node;
        first_node = first_node->next;
        free (t);
    }
}

void*
CCFN(cc_list_iter_deref) (CC_LIST_ITER iter) {
    list_node *node = (list_node*)iter.node;
    return CC_VPTR_ADD(node, CC_OFFSETOF(list_node, data));
}

CC_LIST_ITER
CCFN(cc_list_iter_prev) (CC_LIST_ITER iter) {
    CC_LIST_ITER ret;
    list_node *node = (list_node*)iter.node;
    ret.node = node->prev;
    return ret;
}

CC_LIST_ITER
CCFN(cc_list_iter_next) (CC_LIST_ITER iter) {
    CC_LIST_ITER ret;
    list_node *node = (list_node*)iter.node;
    ret.node = node->next;
    return ret;
}

_Bool
CCFN(cc_list_iter_eq) (CC_LIST_ITER iter1, CC_LIST_ITER iter2) {
    return iter1.node == iter2.node;
}

CC_LIST_ITER
CCFN(cc_list_find) (const CC_LIST *list,
                    const void *value,
                    _Bool (*cmp)(const void*, const void*)) {
    list_node *head_node = (list_node*)list;
    for (list_node *node = head_node->next;
         node != head_node;
         node = node->next) {
        if (cmp(node->data, value)) {
            CC_LIST_ITER ret;
            ret.node = node;
            return ret;
        }
    }

    CC_LIST_ITER ret;
    ret.node = head_node;
    return ret;
}

CC_LIST_ITER
CCFN(cc_list_find_value) (const CC_LIST *list,
                          const void *value) {
    size_t elem_size = list->elem_size;
    list_node *head_node = (list_node*)list;
    for (list_node *node = head_node->next;
         node != head_node;
         node = node->next) {
        if (memcmp(node->data, value, elem_size) == 0) {
            CC_LIST_ITER ret;
            ret.node = node;
            return ret;
        }
    }

    CC_LIST_ITER ret;
    ret.node = head_node;
    return ret;
}

CC_LIST_ITER
CCFN(cc_list_iter_find) (CC_LIST *list,
                         CC_LIST_ITER first,
                         CC_LIST_ITER last,
                         const void *value,
                         _Bool (*cmp)(const void*, const void*)) {
    (void)list;
    for(; !CCFN(cc_list_iter_eq) (first, last);
        first = CCFN(cc_list_iter_next) (first)) {
        if (cmp(CCFN(cc_list_iter_deref) (first), value)) {
            return first;
        }
    }
    return first;
}

CC_LIST_ITER
CCFN(cc_list_iter_find_value) (CC_LIST *list,
                               CC_LIST_ITER first,
                               CC_LIST_ITER last,
                               const void *value) {
    size_t elem_size = list->elem_size;
    for(; !CCFN(cc_list_iter_eq) (first, last);
        first = CCFN(cc_list_iter_next) (first)) {
        if (memcmp(CCFN(cc_list_iter_deref) (first),
                      value,
                      elem_size) == 0) {
            return first;
        }
    }
    return first;
}
