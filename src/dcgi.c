#include "dcgi.h"

#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "util.h"

void handleDCGI(const char *dcgiLib,
                HttpRequest *request,
                FILE *response,
                Error *error) {
  void *libHandle = dlopen(dcgiLib, RTLD_NOW);
  if (libHandle == NULL) {
    QUICK_ERROR2(error, 500, "error opening DCGI library \"%s\": %s",
                 dcgiLib, dlerror());
    return;
  }

  void *handler = dlsym(libHandle, "dcgi_main");
  if (handler == NULL) {
    goto close_handle_ret;
  }

  dcgi_Function *function = (dcgi_Function*)handler;
  StringPair sentry = (StringPair){ NULL, NULL };
  ccVecPushBack(&request->params, &sentry);
  ccVecPushBack(&request->headers, &sentry);

  StringPair *headerDest = NULL;
  char *dataDest = NULL;
  char *errDest = NULL;
  
  int res = function(request->requestPath,
                     (const StringPair*)ccVecData(&request->headers),
                     (const StringPair*)ccVecData(&request->params),
                     request->body,
                     &headerDest, &dataDest, &errDest);
  if (res != 0 && res != 200) {
    if (errDest != NULL) {
      QUICK_ERROR2(error, 500, "error running DCGI function: %s",
                   errDest);
      free(errDest);
    } else {
      QUICK_ERROR(error, 500, "error running DCGI function");
    }
    goto close_handle_ret;
  }

  size_t contentLength = 0;
  if (dataDest != NULL) {
    contentLength = strlen(dataDest);
  }

  fprintf(response,
          "HTTP/1.1 200 Ok\r\n"
          "Content-Encoding: identity\r\n"
          "Content-Length: %zu\r\n"
          "Connection: close\r\n"
          "Server: %s\r\n",
          contentLength, CHTTPD_SERVER_NAME);
  for (size_t i = 0; headerDest[i].first != NULL; i++) {
    if (stricmp(headerDest[i].first, "Content-Length")) {
      LOG_WARN("Manually setting \"Content-Length\", ignored");
    } else if (stricmp(headerDest[i].first, "")) {
      LOG_WARN("Manually setting \"Connection\", ignored");
    } else {
      fprintf(response, "%s: %s\r\n",
              headerDest[i].first, headerDest[i].second);
    }
    free(headerDest[i].first);
    free(headerDest[i].second);
  }
  free(headerDest);
  
  if (dataDest != NULL) {
    fprintf(response, "\r\n%s", dataDest);
    free(dataDest);
  }

close_handle_ret:
  if (dlclose(libHandle) != 0) {
    if (!isError(error)) {
      QUICK_ERROR2(error, 500, "error closing DCGI library: %s", 
                   dlerror());
    }
  }
}
