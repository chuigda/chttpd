#ifndef UTIL_H
#define UTIL_H

/* Used to indicate generic parameter */
#ifndef TP
#define TP(...)
#endif

typedef struct st_string_pair {
  char *first;
  char *second;
} StringPair;

char* copyString(const char *src);
StringPair copyStringPair(StringPair src);

#endif /* UTIL_H */
