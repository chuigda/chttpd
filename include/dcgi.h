#ifndef CHTTPD_DCGI_H
#define CHTTPD_DCGI_H

#include <stdio.h>
#include "error.h"
#include "http.h"

typedef int (dcgi_Function)(const char *queryPath,
                            const StringPair *headers,
                            const StringPair *params,
                            const char *body,
                            StringPair **headerDest,
                            char **dataDest,
                            char **errDest);
void handleDCGI(const char *dcgiLib,
                HttpRequest *httpRequest,
                FILE *response,
                Error *error);

#endif /* CHTTPD_DCGI_H */
