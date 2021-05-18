#include "cc_vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CC_VEC CCTY(cc_vec)

static void cc_vec_grow(CC_VEC *vec) {
    assert(vec->end == vec->usage);
    size_t length = CC_VPTR_DIFF(vec->end, vec->start);
    size_t target =
        (length == 0) ? CC_VEC_INIT_SIZE * vec->elem_size : length * 2;
    void * start = realloc (vec->start, target);
    assert(start != NULL);
    vec->start = start;
    vec->usage = CC_VPTR_ADD(start, length);
    vec->end   = CC_VPTR_ADD(start, target);
}

void
CCFN(cc_vec_init) (CC_VEC *vec, size_t elem_size) {
    assert(elem_size > 0);
    vec->start     = NULL;
    vec->usage     = NULL;
    vec->end       = NULL;
    vec->elem_size = elem_size;
}

void
CCFN(cc_vec_destroy) (CC_VEC *vec) {
    free(vec->start);
    vec->start     = NULL;
    vec->usage     = NULL;
    vec->end       = NULL;
    vec->elem_size = 0;
}

void
CCFN(cc_vec_push_back) (CC_VEC *vec, const void *data) {
    assert(data != NULL);
    assert(vec != NULL);
    if (vec->usage == vec->end) cc_vec_grow(vec);
    assert(vec->usage < vec->end);
    memcpy(vec->usage, data, vec->elem_size);
    vec->usage = CC_VPTR_ADD(vec->usage, vec->elem_size);
};

void
CCFN(cc_vec_push_front) (CC_VEC *vec, const void *data) {
    assert(data != NULL);
    assert(vec != NULL);
    if (vec->usage == vec->end) cc_vec_grow(vec);
    assert(vec->usage < vec->end);
    memmove(CC_VPTR_ADD(vec->start, vec->elem_size),
               vec->start,
               CC_VPTR_DIFF(vec->usage, vec->start));
    memcpy(vec->start, data, vec->elem_size);
    vec->usage = CC_VPTR_ADD(vec->usage, vec->elem_size);
}

void
CCFN(cc_vec_pop_back) (CC_VEC *vec) {
    assert(vec->usage != vec->start);
    vec->usage = CC_VPTR_SUB(vec->usage, vec->elem_size);
}

void
CCFN(cc_vec_pop_front) (CC_VEC *vec) {
    assert(vec->usage != vec->start);
    memmove(vec->start,
               CC_VPTR_ADD(vec->start, vec->elem_size),
               CC_VPTR_DIFF(vec->usage, vec->start) - vec->elem_size);
    vec->usage = CC_VPTR_SUB(vec->usage, vec->elem_size);
}

void*
CCFN(cc_vec_front) (const CC_VEC *vec) {
    assert(vec->usage != vec->start);
    return vec->start;
}

void*
CCFN(cc_vec_back) (const CC_VEC *vec) {
    assert(vec->usage != vec->start);
    return CC_VPTR_SUB(vec->usage, vec->elem_size);
}

void*
CCFN(cc_vec_nth) (const CC_VEC *vec, size_t idx) {
    assert(vec->usage != vec->start);
    assert(CC_VPTR_ADD(vec->start, idx * vec->elem_size)
              <= vec->usage);
    return CC_VPTR_ADD(vec->start, idx * vec->elem_size);
}

void
CCFN(cc_vec_insert) (CC_VEC *vec, size_t idx, const void *data) {
    assert(data != NULL);
    assert(vec != NULL);
    assert(CC_VPTR_ADD(vec->start, idx * vec->elem_size)
              <= vec->usage);
    if (vec->usage == vec->end) cc_vec_grow(vec);
    void * start_pos = CC_VPTR_ADD(vec->start, idx * vec->elem_size);
    memmove(CC_VPTR_ADD(start_pos, vec->elem_size),
               start_pos,
               CC_VPTR_DIFF(vec->usage, start_pos));
    memcpy(start_pos, data, vec->elem_size);
    vec->usage = CC_VPTR_ADD(vec->usage, vec->elem_size);
}

void
CCFN(cc_vec_remove) (CC_VEC *vec, size_t idx) {
    assert(vec != NULL);
    assert(CC_VPTR_ADD(vec->start, idx * vec->elem_size)
              <= vec->usage);
    void *start_pos = CC_VPTR_ADD(vec->start, idx * vec->elem_size);
    memmove(CC_VPTR_SUB(start_pos, vec->elem_size),
               start_pos,
               CC_VPTR_DIFF(vec->usage, start_pos));
    vec->usage = CC_VPTR_SUB(vec->usage, vec->elem_size);
}

void
CCFN(cc_vec_remove_n) (CC_VEC *vec, size_t first, size_t n) {
    assert(vec != NULL);
    assert(CC_VPTR_ADD(vec->start, (first + n) * vec->elem_size)
              <= vec->usage);
    if (n == 0) return;
    void *start_pos = CC_VPTR_ADD(vec->start, first * vec->elem_size);
    void *end_pos   = CC_VPTR_ADD(start_pos, n * vec->elem_size);
    memmove(start_pos,
               end_pos,
               CC_VPTR_DIFF(vec->usage, end_pos));
    vec->usage = CC_VPTR_SUB(vec->usage, n * vec->elem_size);
}

ssize_t
CCFN(cc_vec_find_in) (const CCTY(cc_vec) *vec,
                      size_t first,
                      ssize_t last,
                      const void *data,
                      _Bool (*cmp)(const void*, const void*)) {
    if (last < 0) {
        last = CCFN(cc_vec_len) (vec);
    }
    (void)first;
    (void)last;
    (void)data;
    (void)cmp;
    return -1;
}

ssize_t
CCFN(cc_vec_find) (const CCTY(cc_vec) *vec,
                   const void *data,
                   _Bool (*cmp)(const void*, const void*)) {
    return CCFN(cc_vec_find_in) (vec,
                                 0,
                                 CCFN(cc_vec_len) (vec),
                                 data,
                                 cmp);
}

ssize_t
CCFN(cc_vec_find_value_in) (const CCTY(cc_vec) *vec,
                            size_t first,
                            ssize_t last,
                            const void *data);

ssize_t
CCFN(cc_vec_find_value) (const CCTY(cc_vec) *vec,
                         const void *data);

size_t
CCFN(cc_vec_len) (const CC_VEC *vec) {
    return CC_VPTR_DIFF(vec->usage, vec->start) / vec->elem_size;
}

size_t
CCFN(cc_vec_size) (const CC_VEC *vec) {
    return CCFN(cc_vec_len) (vec);
}

_Bool
CCFN(cc_vec_empty) (const CC_VEC *vec) {
    return vec->usage == vec->start;
}

void*
CCFN(cc_vec_data) (CC_VEC *vec) {
  return vec->start;
}

const void*
CCFN(cc_vec_data_const) (const CC_VEC *vec) {
  return vec->start;
}

