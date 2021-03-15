#ifndef CCLIB_VEC_H
#define CCLIB_VEC_H

#include <stddef.h>
#include <unistd.h>

#include "cc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CC_VEC_INIT_SIZE 8
typedef struct st_cc_vec {
    void *start;
    void *usage;
    void *end;
    size_t elem_size;
} CCTY(cc_vec);

void
CCFN(cc_vec_init) (CCTY(cc_vec) *vec, size_t elem_size);

void
CCFN(cc_vec_destroy) (CCTY(cc_vec) *vec);

void
CCFN(cc_vec_push_back) (CCTY(cc_vec) *vec, const void *data);

void
CCFN(cc_vec_push_front) (CCTY(cc_vec) *vec, const void *data);

void
CCFN(cc_vec_pop_back) (CCTY(cc_vec) *vec);

void
CCFN(cc_vec_pop_front) (CCTY(cc_vec) *vec);

void*
CCFN(cc_vec_front) (const CCTY(cc_vec) *vec);

void*
CCFN(cc_vec_back) (const CCTY(cc_vec) *vec);

void*
CCFN(cc_vec_nth) (const CCTY(cc_vec) *vec, size_t idx);

void
CCFN(cc_vec_insert) (CCTY(cc_vec) *vec,
                     size_t idx,
                     const void *data);

void
CCFN(cc_vec_remove) (CCTY(cc_vec) *vec, size_t idx);

void
CCFN(cc_vec_remove_n) (CCTY(cc_vec) *vec,
                       size_t first,
                       size_t n);

ssize_t
CCFN(cc_vec_find_in) (const CCTY(cc_vec) *vec,
                      size_t first,
                      ssize_t last,
                      const void *data,
                      _Bool (*cmp)(const void*, const void*));

ssize_t
CCFN(cc_vec_find) (const CCTY(cc_vec) *vec,
                   const void *data,
                   _Bool (*cmp)(const void*, const void*));

ssize_t
CCFN(cc_vec_find_value_in) (const CCTY(cc_vec) *vec,
                            size_t first,
                            ssize_t last,
                            const void *data);

ssize_t
CCFN(cc_vec_find_value) (const CCTY(cc_vec) *vec,
                         const void *data);

size_t
CCFN(cc_vec_len) (const CCTY(cc_vec) *vec);

size_t
CCFN(cc_vec_size) (const CCTY(cc_vec) *vec);

_Bool
CCFN(cc_vec_empty) (const CCTY(cc_vec) *vec);

void
CCFN(cc_vec_shrink) (CCTY(cc_vec) *vec);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCLIB_VEC_H */
