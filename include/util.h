#ifndef UTIL_H
#define UTIL_H

/* Used to indicate generic parameter */
#ifndef TP
#define TP(...)
#endif

/* Used to indicate lifetime or ownership */
#ifndef LT
#define LT(...)
#endif

#ifndef Owned
#define Owned(T) T
#endif

#ifndef Ref
#define Ref(T) T
#endif

typedef struct st_string_pair {
  const char *first;
  const char *second;
} StringPair;

char *copyString(const char *src);

#endif /* UTIL_H */
