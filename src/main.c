#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfglang.h"
#include "file_util.h"
#include "util.h"

#define LARGE_BUFFER_SIZE 65536
#define SMALL_BUFFER_SIZE 4096

static int httpMainLoop(const Config *config);

int main(int argc, const char *argv[]) {
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

  pl2b_Error *error = pl2b_errorBuffer(SMALL_BUFFER_SIZE);
  pl2b_Program cfgProgram =
    pl2b_parse(buffer, SMALL_BUFFER_SIZE, error);
  if (pl2b_isError(error)) {
    LOG_FATAL("cannot parse config file \"%s\": %d: %s",
              argv[1], error->errorCode, error->reason);
    return -1;
  }

  const pl2b_Language *cfgLang = getCfgLanguage();
  pl2b_runWithLanguage(&cfgProgram, cfgLang, &config, error);
  if (pl2b_isError(error)) {
    LOG_FATAL("cannot evaluate config file \"%s\": %d: %s",
              argv[1], error->errorCode, error->reason);
    return -1;
  }

  LOG_INFO("chttpd listening to: %s:%d", config.address, config.port);
  LOG_INFO(" - max pending count to %d", config.maxPending);
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
  pl2b_dropError(error);
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

  listen(fdSock, config->maxPending);

  struct sockaddr_in clientAddr;
  size_t clientAddrSize = sizeof(clientAddr);
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

    const char *placeholder = "HTTP/1.1 200 OK\r\n\r\nplaceholder";
    write(fdConnection, placeholder, strlen(placeholder));
    close(fdConnection);
  }

  return 0;
}

