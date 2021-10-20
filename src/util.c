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

void dropStringPair(StringPair pair) {
  free(pair.first);
  free(pair.second);
}

_Bool strcmp_icase(const char *lhs, const char *rhs) {
  while (*lhs != '\0' && *lhs != '\0') {
    if (tolower(*lhs) != tolower(*rhs)) {
      return 0;
    }
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

_Bool slicecmp_icase(const char *begin, const char *end, const char *rhs) {
  while (begin != end && *rhs != '\0') {
    if (tolower(*begin) != tolower(*rhs)) {
      return 0;
    }
    ++begin;
    ++rhs;
  }
  return begin == end && *rhs == '\0';
}

_Bool urlcmp(const char *url, const char *pattern) {
  if (pattern[0] == '!') {
    return !strcmp(url, pattern + 1);
  }

  while (*url != '\0' && *pattern != '\0') {
    if (*url != *pattern) {
      return 0;
    }
    ++url;
    ++pattern;
  }

  if (*pattern == '\0') {
    return 1;
  }

  return 0;
}

_Bool urlcmp_icase(const char *url, const char *pattern) {
  if (pattern[0] == '!') {
    return strcmp_icase(url, pattern + 1);
  }

  while (*url != '\0' && *pattern != '\0') {
    if (tolower(*url) != tolower(*pattern)) {
      return 0;
    }
    ++url;
    ++pattern;
  }

  if (*pattern == '\0') {
    return 1;
  }

  return 0;
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

static _Thread_local ssize_t workerId = -1;

void setWorkerId(size_t newWorkerId) {
  workerId = newWorkerId;
}

void chttpdLog(LogLevel logLevel,
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

  if (workerId != -1) {
    fprintf(stderr,
            "\033[%sm [ %s %s:%s:%d ] <%zi> %s \033[0m\n",
            log_level_controls[logLevel],
            log_level_texts[logLevel],
            fileName,
            func,
            line,
            workerId,
            buffer);
  } else {
    fprintf(stderr,
            "\033[%sm [ %s %s:%s:%d ] %s \033[0m\n",
            log_level_controls[logLevel],
            log_level_texts[logLevel],
            fileName,
            func,
            line,
            buffer);
  }
  
  if (workerId != -1) {
    free(buffer);
    buffer = NULL;
    bufferSize = 0;
  }
}

