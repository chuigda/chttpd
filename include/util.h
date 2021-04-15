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
_Bool stricmp(const char *lhs, const char *rhs);

typedef enum e_log_level {
  LL_DEBUG = 0,
  LL_INFO  = 1,
  LL_WARN  = 2,
  LL_ERROR = 3,
  LL_FATAL = 4
} LogLevel;

void chttpd_log(LogLevel logLevel,
                const char *fileName,
                int line,
                const char *func,
                const char *fmt,
                ...);

#define LOG(LL, FMT, ...) \
  { chttpd_log(LL, __FILE__, __LINE__, __func__, FMT, ##__VA_ARGS__); }

#endif /* UTIL_H */
