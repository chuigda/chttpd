#ifndef BUFREAD_H
#define BUFREAD_H

#include <stddef.h>

typedef struct st_buf_reader {
  /* implementation omitted */
  int dummy;
} BufReader;

BufReader *openBufReader(int fd, size_t bufferSize);
void closeBufReader(BufReader *reader);

int bufGetChar(BufReader *reader);
size_t bufRead(BufReader *reader, size_t bufSize, unsigned char *buf);

_Bool bufEndOfFile(const BufReader *reader);

#endif /* BUFREAD_H */

