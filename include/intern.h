#ifndef CHTTPD_INTERN_H
#define CHTTPD_INTERN_H

#include <stdio.h>

#include "error.h"

extern const char *ERROR_PAGE_403_CONTENT;
extern const char *ERROR_PAGE_404_CONTENT;
extern const char *ERROR_PAGE_405_CONTENT;
extern const char *ERROR_PAGE_500_CONTENT_PART1;
extern const char *ERROR_PAGE_500_CONTENT_PART2;

extern const char *GENERAL_HEADERS;

extern const char *ERROR_PAGE_403_HEAD;
extern const char *ERROR_PAGE_404_HEAD;
extern const char *ERROR_PAGE_405_HEAD;
extern const char *ERROR_PAGE_500_HEAD;

void send403Page(FILE *fp);
void send404Page(FILE *fp);
void send405Page(FILE *fp);
void send500Page(FILE *fp, Error *reason);

void sendOptionsAcceptedPage(FILE *fp, unsigned allowedMethods);

void handleIntern(const char *handlerPath, Error *error);

#endif /* CHTTPD_INTERN_H */

