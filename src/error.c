#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Error *errorBuffer(uint16_t bufferSize) {
  Error *ret = (Error*)malloc(sizeof(Error) + bufferSize);
  ret->sourceInfo = (SourceInfo) { NULL, -1 };
  ret->errCode = 0;
  ret->bufferSize = bufferSize;

  memset(ret->errorBuffer, 0, bufferSize);

  return ret;
}

void dropError(Error *error) {
  free(error);
}

_Bool isError(Error *error) {
  return error->errCode != 0;
}

void formatError(Error *error,
                 SourceInfo sourceInfo,
                 int16_t errCode,
                 const char *fmt,
                 ...) {
  error->sourceInfo = sourceInfo;
  error->errCode = errCode;
  if (error->bufferSize == 0) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(error->errorBuffer, error->bufferSize, fmt, ap);
  va_end(ap);
}

