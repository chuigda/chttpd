#include "route.h"

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "chttpd_cfg.h"
#include "error.h"
#include "file_util.h"
#include "util.h"

static const char *mimeGuess(const char *filePath);
static ccVec TP(char*) requestToEnv(const Config *config,
                                    const HttpRequest *request,
                                    const char *clientAddr,
                                    const char *scriptName);
static char *buildEnvItem(const char *key, const char *value);

static void handleStatic(const char *filePath,
                         FILE *fp,
                         Error *error);
static void handleCGI(const char *cgiProgram,
                      const HttpRequest *request,
                      const char *clientAddr,
                      FILE *fp,
                      Error *error);

void routeAndHandle(const Config *config,
                    const HttpRequest *request,
                    const char *clientAddr,
                    FILE *fp,
                    Error *error) {
  for (size_t i = 0; i < ccVecLen(&config->routes); i++) {
    const Route *route = (Route*)ccVecNth(&config->routes, i);
    if (!strcmp(request->requestPath, route->path)
        && request->method == route->httpMethod) {
      switch (route->handlerType) {
      case HDLR_SCRIPT:
        QUICK_ERROR(error, 500, "SCRIPT not supported yet");
        break;
      case HDLR_STATIC:
        handleStatic(route->handlerPath, fp, error);
        break;
      case HDLR_CGI:
        handleCGI(route->handlerPath, request, clientAddr, fp, error);
        break;
      }
      return;
    }
  }
  QUICK_ERROR(error, 404, "");
}

static const char *mimeGuess(const char *filePath) {
  const char *postfix = strrchr(filePath, '.');
  if (postfix == NULL) {
    return "application/octet-stream";
  }

  if (stricmp(postfix, ".html")) {
    return "text/html";
  } else if (stricmp(postfix, ".js")) {
    return "text/javascript";
  } else if (stricmp(postfix, ".css")) {
    return "text/css";
  } else if (stricmp(postfix, ".json")) {
    return "application/json";
  } else if (stricmp(postfix, ".xml")) {
    return "application/xml";
  } else if (stricmp(postfix, ".txt")) {
    return "text/plain";
  }
  
  return "application/octet-stream";
}

static void handleStatic(const char *filePath,
                         FILE *fp,
                         Error *error) {
  FILE *fpFile = fopen(filePath, "r");
  if (fpFile == NULL) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot open file: %s",
                 filePath);
    return;
  }

  ssize_t fileSize = readAll(fpFile, NULL, 0);
  if (fileSize < 0) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot get size of file: %s",
                 filePath);
    return;
  }

  char *buffer = (char*)malloc(fileSize);
  if (buffer == NULL) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot allocate %zi bytes",
                 fileSize);
    return;
  }

  ssize_t bytesRead = readAll(fpFile, buffer, fileSize);
  if (bytesRead != fileSize) {
    QUICK_ERROR2(error, 500,
                 "handleStatic: cannot read %zi bytes from %s",
                 fileSize, filePath);
    return;
  }

  fprintf(fp,
          "HTTP/1.1 200 OK\r\n"
          "Server: %s\r\n"
          "Connection: close\r\n"
          "Content-Encoding: identity\r\n"
          "Content-Type: %s\r\n"
          "Content-Length: %zi\r\n\r\n",
          CHTTPD_SERVER_NAME, mimeGuess(filePath), fileSize);
  if (errno != 0) {
    LOG_WARN("failed to write response header: %d", errno);
    return;
  }

  ssize_t bytesWrite = fwrite(buffer, 1, fileSize, fp);
  if (bytesWrite != fileSize) {
    LOG_WARN("failed to respond %zi bytes: %d", fileSize, errno);
    return;
  }
}

static void handleCGI(const char *cgiPath,
                      const HttpRequest *request,
                      const char *clientAddr,
                      FILE *fp,
                      Error *error) {
  QUICK_ERROR(error, 500, "CGI not implemented yet");
}

static ccVec TP(char*) requestToEnv(const Config *config,
                                    const HttpRequest *request,
                                    const char *clientAddr,
                                    const char *scriptName) {
  char *requestMethod = buildEnvItem("REQUEST_METHOD",
                                     HTTP_METHOD_NAMES[request->method]);
  char *queryString = buildEnvItem("QUERY_STRING", request->queryString);
  char *serverName = buildEnvItem("SERVER_NAME", config->address);
  char *serverSoft = buildEnvItem("SERVER_SOFTWARE", CHTTPD_SERVER_NAME);
  char *scName = buildEnvItem("SCRIPT_NAME", scriptName);

  /* TODO not implemented */
}

static char *buildEnvItem(const char *key, const char *value) {
  char *ret = (char*)malloc(strlen(key) + strlen(value) + 1);
  char *iter = ret;
  while (*key != '\0') {
    if (*key == '-') {
      *iter = '_';
    } else {
      *iter = toupper(*key);
    }
    iter++;
    key++;
  }
  *iter++ = '=';
  strcpy(iter, value);
  return ret;
}

