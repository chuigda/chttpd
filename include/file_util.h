#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdio.h>
#include <unistd.h>

ssize_t readAll(FILE *fp, char *buffer, size_t bufSize);

#endif /* FILE_UTIL_H */
