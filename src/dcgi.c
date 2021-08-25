#include "dcgi.h"

#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "util.h"

DCGIModule *loadDCGIModule(const char *dcgiLib,
                           Error *error) {
  void *libHandle = dlopen(dcgiLib, RTLD_NOW);

  if (libHandle == NULL) {
    QUICK_ERROR2(error, 500, "error opening DCGI library \"%s\": %s",
                 dcgiLib, dlerror());
    return NULL;
  }

  void *dcgiMain = dlsym(libHandle, "dcgi_main");
  if (dcgiMain == NULL) {
    QUICK_ERROR2(error, 500, "error locating 'dcgi_main' on \"%s\": %s",
                 dcgiLib, dlerror());
    if (dlclose(libHandle) != 0) {
      LOG_WARN("cannot close dynamic loaded library handle: %s",
               dlerror());
    }
    return NULL;
  }

  void *dcgiDealloc = dlsym(libHandle, "dcgi_dealloc");
  if (dcgiDealloc == NULL) {
    LOG_WARN("this DCGI module appear to have no \"dcgi_dealloc\"");
    LOG_WARN("this may cause bugs, be cautious!");
  }

  DCGIModule *module = (DCGIModule*)malloc(sizeof(DCGIModule));
  module->dcgiMain = (DCGIMain*)dcgiMain;
  module->dcgiDealloc = (DCGIDealloc*)dcgiDealloc;
  return module;
}

void unloadDCGIModule(DCGIModule *module, Error *error) {
  if (dlclose(module->libHandle) != 0) {
    QUICK_ERROR2(error, 500, "error closing DCGI library: %s",
                 dlerror());
  }
  free(module);
}

void handleDCGI(const char *dcgiLib,
                DCGIModule *preloaded,
                HttpRequest *request,
                FILE *response,
                Error *error) {
  DCGIModule *module = preloaded;
  if (preloaded == NULL) {
    module = loadDCGIModule(dcgiLib, error);
    if (isError(error)) {
      return;
    }
  } else {
    LOG_DBG("using preloaded library");
  }

  StringPair sentry = (StringPair){ NULL, NULL };
  ccVecPushBack(&request->params, &sentry);
  ccVecPushBack(&request->headers, &sentry);

  StringPair *headerDest = NULL;
  char *dataDest = NULL;
  char *errDest = NULL;

  int res = module->dcgiMain(
              request->method,
              request->requestPath,
              (const StringPair*)ccVecData(&request->headers),
              (const StringPair*)ccVecData(&request->params),
              request->body,
              &headerDest,
              &dataDest,
              &errDest
            );
  if (res == 500) {
    if (errDest != NULL) {
      QUICK_ERROR2(error, 500, "error running DCGI function: %s",
                   errDest);
    } else {
      QUICK_ERROR(error, 500, "error running DCGI function");
    }
    goto unload_module_ret;
  }

  size_t contentLength = 0;
  if (dataDest != NULL) {
    contentLength = strlen(dataDest);
  }

  fprintf(response,
          "HTTP/1.1 %d %s\r\n"
          "Content-Encoding: identity\r\n"
          "Content-Length: %zu\r\n"
          "Connection: close\r\n"
          "Server: %s\r\n",
          res,
          httpCodeNameSafe(res),
          contentLength,
          CHTTPD_SERVER_NAME);

  size_t headerCount = 0;
  for (; headerDest[headerCount].first != NULL; headerCount++) {
    const char *headerKey = headerDest[headerCount].first;
    const char *headerValue = headerDest[headerCount].second;
    if (strcmp_icase(headerKey, "Content-Length")) {
      LOG_WARN("Manually setting \"Content-Length\", ignored");
    } else if (strcmp_icase(headerKey, "Connection")) {
      LOG_WARN("Manually setting \"Connection\", ignored");
    } else {
      fprintf(response, "%s: %s\r\n", headerKey, headerValue);
    }
  }
  
  if (dataDest != NULL) {
    fprintf(response, "\r\n%s", dataDest);
  }

  if (module->dcgiDealloc != NULL) {
    DCGIDealloc *dealloc = module->dcgiDealloc;
    for (size_t i = 0; i < headerCount; i++) {
      char *headerKey = headerDest[i].first;
      char *headerValue = headerDest[i].second;
      dealloc(headerKey, strlen(headerKey) + 1, _Alignof(char));
      dealloc(headerValue, strlen(headerValue) + 1, _Alignof(char));
    }
    dealloc(headerDest,
            sizeof(StringPair) * (headerCount + 1),
            _Alignof(StringPair));
    if (dataDest) {
      dealloc(dataDest, strlen(dataDest) + 1, _Alignof(char));
    }
  } else {
    for (size_t i = 0; i < headerCount; i++) {
      free(headerDest[i].first);
      free(headerDest[i].second);
    }
    free(headerDest);
    if (dataDest) {
      free(dataDest);
    }
  }

unload_module_ret:
  if (errDest) {
    if (module->dcgiDealloc) {
      module->dcgiDealloc(errDest, strlen(errDest) + 1, _Alignof(char));
    } else {
      free(errDest);
    }
  }

  if (preloaded == NULL) {
    unloadDCGIModule(module, error);
  }
}

