#ifndef ROUTE_H
#define ROUTE_H

#include "cfglang.h"
#include "error.h"
#include "http.h"

void routeAndHandle(const Config *config,
                    const HttpRequest *request,
                    const char *clientAddr,
                    FILE *fp,
                    Error *error);

#endif /* ROUTE_H */
