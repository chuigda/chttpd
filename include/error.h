#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct st_source_info {
  const char *sourceFile;
  ssize_t line;
} SourceInfo;

typedef struct st_error {
  SourceInfo sourceInfo;
  int16_t errCode;
  uint16_t bufferSize;
  char errorBuffer[0];
} Error;

Error *errorBuffer(uint16_t bufferSize);
void dropError(Error *error);
_Bool isError(Error *error);
void formatError(Error *error,
                 SourceInfo sourceInfo,
                 int16_t errorCode,
                 const char *fmt,
                 ...);

#define QUICK_ERROR(ERR, CODE, FMT) \
  { formatError(ERR, \
                (SourceInfo) { __FILE__, __LINE__ }, \
                CODE, \
                FMT); }

#define QUICK_ERROR2(ERR, CODE, FMT, ...) \
  { formatError(ERR, \
                (SourceInfo) { __FILE__, __LINE__ }, \
                CODE, \
                FMT, \
                __VA_ARGS__ \
               ); }

#endif /* ERROR_H */
