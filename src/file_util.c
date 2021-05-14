#include "file_util.h"
#include "util.h"

#include <stdio.h>

ssize_t readAll(FILE *fp, char *buffer, size_t bufSize) {
  long res;
  if ((res = fseek(fp, 0, SEEK_END)) < 0) {
    LOG_ERR("cannot fseek fp to SEEK_END, error = %d", res);
    return -1;
  }
  if ((res = ftell(fp)) < 0) {
    LOG_ERR("cannot ftell fp, error = %d", res);
    return -1;
  }
  rewind(fp);

  if (buffer == NULL) {
    return res;
  }

  size_t bytesToRead = bufSize < (size_t)res ? bufSize : (size_t)res;
  size_t bytesRead = fread(buffer, 1, bytesToRead, fp);
  if (bytesRead < bytesToRead) {
    res = ferror(fp);
    LOG_ERR("cannot read %d bytes from fp, error = %d", res);
    return -1;
  }

  return bytesRead;
}
