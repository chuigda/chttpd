#include "util.h"

#include <stdlib.h>
#include <string.h>

char *copyString(const char *src) {
  size_t len = strlen(src);

  char *ret = (char*)malloc(len + 1);
  strcpy(ret, src);
  return ret;
}
