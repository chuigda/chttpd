#ifndef CHTTPD_STATIC_H
#define CHTTPD_STATIC_H

#include <stdio.h>
#include "error.h"

void handleStatic(const char *filePath,
                  FILE *fp,
                  Error *error);

#endif /* CHTTPD_STATIC_H */
