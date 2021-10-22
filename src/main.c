#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "dcgi.h"
#include "file_util.h"
#include "http.h"
#include "intern.h"
#include "static.h"
#include "util.h"

#define LARGE_BUFFER_SIZE 65536
#define SMALL_BUFFER_SIZE 4096

typedef struct st_http_input_context {
  size_t workerId;
  const Config *config;
  int fdConnection;
  char *clientAddr;
} HttpInputContext;

typedef _Bool (UrlCompare)(const char*, const char*);

static int httpMainLoop(const Config *config);
static void *httpHandler(void* context);
static void routeAndHandle(const Config *config,
                           HttpRequest *request,
                           FILE *fp,
                           Error *error);
static _Bool isCorsRequest(const HttpRequest *request);
static unsigned getAllowedCorsMethods(const Config *config,
                                      const char *path,
                                      UrlCompare *urlCompare);

int main(int argc, const char *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  if (argc != 2) {
    LOG_FATAL("expected 1 argument, got %d", argc - 1);
    LOG_INFO("usage: chttpd [config-file]");
    return -1;
  }

  FILE *cfg_file = fopen(argv[1], "r");
  if (cfg_file == NULL) {
    LOG_FATAL("cannot open config file \"%s\"", argv[1]);
    return -1;
  }
  char *buffer = (char*)malloc(LARGE_BUFFER_SIZE);
  if (buffer == NULL) {
    LOG_FATAL("failed allocating read buffer");
    return -1;
  }

  memset(buffer, 0, LARGE_BUFFER_SIZE);
  ssize_t bytesRead = readAll(cfg_file, buffer, LARGE_BUFFER_SIZE);
  if (bytesRead < 0) {
    LOG_FATAL("cannot read config file \"%s\"", argv[1]);
    return -1;
  }

  Config config;
  initConfig(&config);

  Error *error = errorBuffer(SMALL_BUFFER_SIZE);
  pl2b_Program cfgProgram =
    pl2b_parse(buffer, SMALL_BUFFER_SIZE, error);
  if (isError(error)) {
    LOG_FATAL("cannot parse config file \"%s\": %d: %s",
              argv[1], error->errCode, error->errorBuffer);
    return -1;
  }

  const pl2b_Language *cfgLang = getCfgLanguage();
  pl2b_runWithLanguage(&cfgProgram, cfgLang, &config, error);
  if (isError(error)) {
    LOG_FATAL("cannot evaluate config file \"%s\": %d: %s",
              argv[1], error->errCode, error->errorBuffer);
    return -1;
  }

  LOG_INFO("chttpd listening to: %s:%d", config.address, config.port);
  LOG_INFO(" - max pending count set to %d", config.maxPending);
  LOG_INFO(" - DCGI preloading %s",
           config.preloadDynamic ? "enabled" : "disabled");
  if (config.cacheTime >= 0) {
    LOG_INFO(" - cache expiration set to %d", config.cacheTime);
  } else {
    LOG_INFO(" - cache disabled");
  }
  LOG_INFO(" - case ignore set to %s",
           config.ignoreCase ? "true" : "false");
  for (size_t i = 0; i < ccVecLen(&config.routes); i++) {
    Route *route = (Route*)ccVecNth(&config.routes, i);
    LOG_INFO(" - route \"%s %s\" to \"%s %s\"",
             HTTP_METHOD_NAMES[route->httpMethod],
             route->path,
             HANDLER_TYPE_NAMES[route->handlerType],
             route->handlerPath);
  }

  int ret = httpMainLoop(&config);

  dropConfig(&config);
  dropError(error);
  pl2b_dropProgram(&cfgProgram);
  free(buffer);

  return ret;
}

static int httpMainLoop(const Config *config) {
  int fdSock;
  struct sockaddr_in serverAddr; 

  if ((fdSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_FATAL("error on opening socket: %d", errno);
    return -1;
  }

  int reuseAddr = 1;
  if (setsockopt(fdSock,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 &reuseAddr,
                 sizeof(int)) < 0) {
    LOG_FATAL("failed setting SO_REUSEADDR: %d", errno);
    return -1;
  }

  struct in_addr listenAddress;
  int res = inet_aton(config->address, &listenAddress);
  if (res == 0) {
    LOG_FATAL("invalid listening address: %s", config->address);
    return -1;
  }

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(config->port);
  serverAddr.sin_addr = listenAddress;

  if (bind(fdSock,
           (struct sockaddr*)&serverAddr,
           sizeof(serverAddr)) < 0) {
    LOG_FATAL("error on binding: %d", errno);
    return -1;
  }

  size_t workerId = 0;
  listen(fdSock, config->maxPending);

  struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  for (;;) {
    int fdConnection = accept(fdSock,
                              (struct sockaddr*)&clientAddr,
                              &clientAddrSize);
    if (fdConnection < 0) {
      LOG_ERR("error on accepting connection: %d", errno);
      return -1;
    }

    char *addrStr = inet_ntoa(clientAddr.sin_addr);
    LOG_INFO("accepting connection from: %s", addrStr);
    
    HttpInputContext *inputContext =
      (HttpInputContext*)malloc(sizeof(HttpInputContext));
    if (inputContext == NULL) {
      LOG_ERR("failed allocating thread context buffer");
      continue;
    }
    inputContext->workerId = workerId++;
    inputContext->config = config;
    inputContext->fdConnection = fdConnection;
    inputContext->clientAddr = copyString(addrStr);

    pthread_t thread;
    int res = pthread_create(&thread, NULL, httpHandler, inputContext);
    if (res != 0) {
      LOG_ERR("error on pthread creation: %d", res);
      continue;
    }

    res = pthread_detach(thread);
    if (res != 0) {
      LOG_ERR("error on pthread detach: %d", res);
    }
  }

  return 0;
}

static void* httpHandler(void *context) {
  HttpInputContext *inputContext = (HttpInputContext*)context;
  setWorkerId(inputContext->workerId);
  const Config *config = inputContext->config;
  int fd = inputContext->fdConnection;

  FILE *fp = fdopen(fd, "a+");
  if (fp == NULL) {
    LOG_ERR("error calling fdopen: %d", errno);
  }

  HttpRequest *request = readHttpRequest(fp);
  if (request == NULL) {
    goto close_fp_ret;
  }

  LOG_INFO("accepting HTTP request: %s %s",
           HTTP_METHOD_NAMES[request->method],
           request->requestPath);
  LOG_INFO(" - ?%s", request->queryString);
  for (size_t i = 0; i < ccVecLen(&request->params); i++) {
    StringPair *param = (StringPair*)ccVecNth(&request->params, i);
    LOG_INFO(" ?%s=%s", param->first, param->second);
  }

  for (size_t i = 0; i < ccVecLen(&request->headers); i++) {
    StringPair *header = (StringPair*)ccVecNth(&request->headers, i);
    LOG_INFO(" %s: \"%s\"", header->first, header->second);
  }
  if (request->contentLength != 0) {
    LOG_INFO("body:\n\n%s", request->body);
  }

  Error *error = errorBuffer(SMALL_BUFFER_SIZE);

  routeAndHandle(config, request, fp, error);
  dropHttpRequest(request);

  if (!isError(error)) {
  } else if (error->errCode == 403) {
    send403Page(fp);
  } else if (error->errCode == 404) {
    send404Page(fp);
  } else {
    send500Page(fp, error);
  }

  dropError(error);

close_fp_ret:
  fflush(fp);
  fclose(fp);
  free(inputContext->clientAddr);
  free(inputContext);
  return NULL;
}

static void routeAndHandle(const Config *config,
                           HttpRequest *request,
                           FILE *fp,
                           Error *error) {
  UrlCompare *urlCompare = 
    config->ignoreCase ? urlcmp_icase : urlcmp;

  size_t routeCount = ccVecLen(&config->routes);
  for (size_t i = 0; i < routeCount; i++) {
    const Route *route = (Route*)ccVecNth(&config->routes, i);
    if (urlCompare(request->requestPath, route->path)
        && request->method == route->httpMethod) {
      switch (route->handlerType) {
      case HDLR_STATIC:
        handleStatic(route->handlerPath, fp, config->cacheTime, error);
        break;
      case HDLR_DCGI:
        handleDCGI(route->handlerPath,
                   (DCGIModule*)route->extra,
                   request,
                   fp,
                   error);
        break;
      case HDLR_INTERN:
        handleIntern(route->handlerPath, error);
        break;
      case HDLR_DIR:
        QUICK_ERROR(error, 500, "DIR not supported yet");
        break;
      }
      return;
    }
  }

  QUICK_ERROR(error, 404, "");
}

static void respondToOptionsRequest(const Config *config,
                                    FILE *fp,
                                    unsigned allowedMethods) {
  if (allowedMethods == 0) {
    send405Page(fp);
  }


}

static _Bool isCorsRequest(const HttpRequest *request) {
  _Bool hasReferer = 0;
  _Bool hasOrigin = 0;

  size_t headerCount = ccVecLen(&request->headers);
  for (size_t i = 0; i < headerCount; i++) {
    StringPair *header = (StringPair*)ccVecNth(&request->headers, i);
    if (strcmp_icase(header->first, "Referer")) {
      hasReferer = 1;
      continue;
    }

    if (strcmp_icase(header->second, "Origin")) {
      hasOrigin = 1;
    }
  }

  return hasReferer && hasOrigin;
}
  
static unsigned getAllowedCorsMethods(const Config *config,
                                      const char *path,
                                      UrlCompare *urlCompare) {
  unsigned ret = 0;

  size_t corsConfigCount = ccVecLen(&config->corsConfig);
  for (size_t i = 0; i < corsConfigCount; i++) {
    const CorsConfig *configItem =
      (const CorsConfig*)ccVecNth(&config->corsConfig, i);
    if (urlCompare(path, configItem->path)) {
      ret |= configItem->httpMethod;
    }
  }
  return ret;
}

