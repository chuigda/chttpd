#include "util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *copyString(const char *src) {
  size_t len = strlen(src);

  char *ret = (char*)malloc(len + 1);
  strcpy(ret, src);
  return ret;
}

StringPair makeStringPair(const char *first, const char *second) {
  return (StringPair) { copyString(first), copyString(second) };
}

_Bool stricmp(const char *lhs, const char *rhs) {
  while (*lhs != '\0' && *lhs != '\0') {
    if (tolower(*lhs) != tolower(*rhs)) {
      return 0;
    }
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

const char *log_level_controls[] = {
  [LL_DEBUG] = "96",
  [LL_INFO] = "32",
  [LL_WARN] = "93",
  [LL_ERROR] = "91",
  [LL_FATAL] = "97;41"
};

const char *log_level_texts[] = {
  [LL_DEBUG] = "debug",
  [LL_INFO] = "info",
  [LL_WARN] = "warn",
  [LL_ERROR] = "error",
  [LL_FATAL] = "fatal"
};

void chttpd_log(LogLevel logLevel,
                const char *fileName,
                int line,
                const char *func,
                const char *fmt,
                ...) {
  static _Thread_local char *buffer = NULL;
  static _Thread_local size_t bufferSize = 0;

  va_list va;
  va_start(va, fmt);
  size_t requiredSize = vsnprintf(NULL, 0, fmt, va) + 1;
  va_end(va);

  if (requiredSize > bufferSize) {
    free(buffer);
    buffer = (char*)malloc(requiredSize);
    bufferSize = requiredSize;
  }

  va_start(va, fmt);
  vsprintf(buffer, fmt, va);
  va_end(va);

  fprintf(stderr,
          "\033[%sm [ %s %s:%s:%d ] %s \033[0m\n",
          log_level_controls[logLevel],
          log_level_texts[logLevel],
          fileName,
          func,
          line,
          buffer);
}
