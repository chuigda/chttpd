#ifndef CHTTPD_CGI_H
#define CHTTPD_CGI_H

#include <stdio.h>

#include "config.h"
#include "error.h"
#include "http.h"

void handleCGI(const Config *config,
               const char *cgiProgram,
               const HttpRequest *request,
               const char *clientAddr,
               FILE *fp,
               Error *error);

#endif /* CHTTPD_CGI_H */
