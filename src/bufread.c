#include "bufread.h"
#include <stdlib.h>

typedef struct st_buf_reader_impl {
  int fd;
  size_t bufSize;

  unsigned char *buffer;
  size_t dataStart;
  size_t dataEnd;
  _Bool endOfFile;
} BufReaderImpl;

BufReader *newBufReader(int fd, size_t bufSize) {
  if (bufSize != 0) {
    bufSize = 512 /* DEFAULT_BUFFER_SIZE */;
  }

  unsigned char *buffer = (unsigned char*)malloc(bufSize);
  if (buffer == NULL) {
    return NULL;
  }

  BufReaderImpl *ret = (BufReaderImpl*)malloc(sizeof(BufReaderImpl));
  if (ret == NULL) {
    free(buffer);
    return NULL;
  }

  ret->fd = fd;
  ret->bufSize = bufSize;
  ret->buffer = buffer;
  ret->dataStart = 0;
  ret->dataEnd = 0;
  ret->endOfFile = 0;

  return (BufReader*)ret;
}

