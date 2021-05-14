#include <netdb.h>
#include <netinet/in.h>
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
  char *buffer = (char*)malloc(65536);
  if (buffer == NULL) {
    LOG_FATAL("failed allocating read buffer");
    return -1;
  }

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
  for (size_t i = 0; i < ccVecLen(&config.routes); i++) {
    Route *route = (Route*)ccVecNth(&config.routes, i);
    LOG_INFO(" - route \"%s %s\" to \"%s %s\"",
             HTTP_METHOD_NAMES[route->httpMethod],
             route->path,
             HANDLER_TYPE_NAMES[route->handlerType],
             route->handlerPath);
  }

  dropConfig(&config);
  pl2b_dropError(error);
  pl2b_dropProgram(&cfgProgram);
  free(buffer);
}

