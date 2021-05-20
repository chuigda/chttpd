#ifndef CHTTPD_ROUTE_H
#define CHTTPD_ROUTE_H

#include "config.h"
#include "error.h"
#include "http.h"

void routeAndHandle(const Config *config,
                    const HttpRequest *request,
                    const char *clientAddr,
                    FILE *fp,
                    Error *error);

#endif /* CHTTPD_ROUTE_H */
