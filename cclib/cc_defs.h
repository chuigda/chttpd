#ifndef CCLIB_DEFS_H
#define CCLIB_DEFS_H

#ifndef CCTY       /* PROC_MACRO_FALLBACK */
#define CCTY(X) X  /* PROC_MACRO_FALLBACK */
#endif             /* PROC_MACRO_FALLBACK */

#ifndef CCFN       /* PROC_MACRO_FALLBACK */
#define CCFN(X) X  /* PROC_MACRO_FALLBACK */
#endif             /* PROC_MACRO_FALLBACK */

#define CC_VPTR_DIFF(x, y) (ssize_t)(x) - (ssize_t)(y))
#define CC_OFFSETOF(a,b) (ssize_t)(&(((a*)(0))->b))
#define CC_VPTR_ADD(ptr, delta) \
    ((void *)((size_t)(ptr) + (size_t)(delta)))

#endif /* CCLIB_DEFS_H */
