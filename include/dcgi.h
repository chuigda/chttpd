#ifndef CHTTPD_DCGI_H
#define CHTTPD_DCGI_H

#include <stdio.h>
#include "error.h"
#include "http.h"

typedef int (DCGIMain)(int requestMethod,
		                   const char *queryPath,
                       const StringPair *headers,
                       const StringPair *params,
                       const char *body,
                       StringPair **headerDest,
                       char **dataDest,
                       char **errDest);

typedef void (DCGIDealloc)(void *ptr, int size, int align);

typedef struct st_dcgi_handler_extras {
  void *libHandle;
  DCGIMain *dcgiMain;
  DCGIDealloc *dcgiDealloc;
} DCGIModule;

DCGIModule *loadDCGIModule(const char *dcgiLib, Error *error);

void unloadDCGIModule(DCGIModule *module, Error *error);

void handleDCGI(const char *dcgiLib,
                DCGIModule *preloaded,
                HttpRequest *httpRequest,
                FILE *response,
                Error *error);

#endif /* CHTTPD_DCGI_H */
