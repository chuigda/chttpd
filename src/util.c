#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *copyString(const char *src) {
  size_t len = strlen(src);

  char *ret = (char*)malloc(len + 1);
  strcpy(ret, src);
  return ret;
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
