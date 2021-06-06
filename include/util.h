#ifndef CHTTPD_UTIL_H
#define CHTTPD_UTIL_H

#include <stdlib.h>

/* Used to indicate generic parameter */
#ifndef TP
#define TP(...)
#endif

typedef struct st_string_pair {
  char *first;
  char *second;
} StringPair;

char* copyString(const char *src);
StringPair makeStringPair(const char *first, const char *second);
StringPair copyStringPair(StringPair src);
void dropStringPair(StringPair pair);

_Bool strcmp_icase(const char *lhs, const char *rhs);

typedef enum e_log_level {
  LL_DEBUG = 0,
  LL_INFO  = 1,
  LL_WARN  = 2,
  LL_ERROR = 3,
  LL_FATAL = 4
} LogLevel;

void setWorkerId(size_t workerId);
void chttpdLog(LogLevel logLevel,
               const char *fileName,
               int line,
               const char *func,
               const char *fmt,
               ...);

#define LOG(LL, FMT, ...) \
  { chttpdLog(LL, __FILE__, __LINE__, __func__, FMT, ##__VA_ARGS__); }

#define LOG_DBG(FMT, ...) { LOG(LL_DEBUG, FMT, ##__VA_ARGS__); }
#define LOG_INFO(FMT, ...) { LOG(LL_INFO, FMT, ##__VA_ARGS__); }
#define LOG_WARN(FMT, ...) { LOG(LL_WARN, FMT, ##__VA_ARGS__); }
#define LOG_ERR(FMT, ...) { LOG(LL_ERROR, FMT, ##__VA_ARGS__); }
#define LOG_FATAL(FMT, ...) { LOG(LL_FATAL, FMT, ##__VA_ARGS__); }

#endif /* CHTTPD_UTIL_H */
