#include "config.h"
#include "dcgi.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ADDRESS         "127.0.0.1"
#define DEFAULT_PORT            8080
#define DEFAULT_MAX_PENDING     16
#define DEFAULT_PRELOAD_DYNAMIC 0
#define DEFAULT_IGNORE_CASE     1
#define DEFAULT_CACHE_TIME      (-1)

const char *HANDLER_TYPE_NAMES[] = {
  [HDLR_STATIC] = "STATIC",
  [HDLR_DCGI]   = "DCGI",
  [HDLR_INTERN] = "INTERN",
  [HDLR_DIR]    = "DIR"
};

void initConfig(Config *config) {
  config->address = DEFAULT_ADDRESS;
  config->port = DEFAULT_PORT;
  config->maxPending = DEFAULT_MAX_PENDING;
  config->preloadDynamic = DEFAULT_PRELOAD_DYNAMIC;
  config->ignoreCase = DEFAULT_IGNORE_CASE;
  config->cacheTime = DEFAULT_CACHE_TIME;
  ccVecInit(&config->routes, sizeof(Route));
}

void dropConfig(Config *config) {
  ccVecDestroy(&config->routes);
}

static pl2b_Cmd* configAddr(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error);

static pl2b_Cmd *configIntAttr(pl2b_Program *program,
                               int *dest,
                               pl2b_Cmd *command,
                               Error *error,
                               int lowerBound,
                               int upperBound);

static pl2b_Cmd *configBoolAttr(pl2b_Program *program,
                                _Bool *dest,
                                pl2b_Cmd *command,
                                Error *error);

static pl2b_Cmd *configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error);

static pl2b_Cmd *configPend(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error);

static pl2b_Cmd *configPreloadDyn(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error);

static pl2b_Cmd *configIgnoreCase(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error);

static pl2b_Cmd *configCacheTime(pl2b_Program *program,
                                 void *context,
                                 pl2b_Cmd *command,
                                 Error *error);

static pl2b_Cmd *addRoute(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          Error *error);

const pl2b_Language *getCfgLanguage(void) {
  static pl2b_PCallCmd pCallCmds[] = {
    { "listen-address", NULL, configAddr,       0, 0 },
    { "listen-port",    NULL, configPort,       0, 0 },
    { "max-pending",    NULL, configPend,       0, 0 },
    { "preload",        NULL, configPreloadDyn, 0, 0 },
    { "ignore-case",    NULL, configIgnoreCase, 0, 0 },
    { "cache-time",     NULL, configCacheTime,  0, 0 },
    { "post",           NULL, addRoute,         0, 0 },
    { "POST",           NULL, addRoute,         0, 0 },
    { "Post",           NULL, addRoute,         0, 0 },
    { "get",            NULL, addRoute,         0, 0 },
    { "Get",            NULL, addRoute,         0, 0 },
    { "GET",            NULL, addRoute,         0, 0 },
    { NULL, NULL, NULL, 0, 0 }
  };

  static const pl2b_Language language = (pl2b_Language) {
    "chttpd cfg language",
    "configuration language for chttpd",
    pCallCmds,
    NULL
  };

  return &language;
}

static pl2b_Cmd* configAddr(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error) {
  (void)program;

  Config *config = (Config*)context;

  if (pl2b_argsLen(command) != 1) {
    formatError(error, command->sourceInfo, -1,
                "listen-address: expects exactly one argument");
    return NULL;
  }

  config->address = command->args[0].str;
  return command->next;
}

static pl2b_Cmd *configIntAttr(pl2b_Program *program,
                               int *dest,
                               pl2b_Cmd *command,
                               Error *error,
                               int lowerBound,
                               int upperBound) {
  (void)program;

  if (pl2b_argsLen(command) != 1) {
    formatError(error, command->sourceInfo, -1,
                "%s: expects exactly one argument",
                command->cmd.str);
    return NULL;
  }

  int value = atoi(command->args[0].str);
  if ((lowerBound != INT_MIN && value <= lowerBound)
      || (upperBound != INT_MIN && value >= upperBound)) {
    formatError(error, command->sourceInfo, -1,
                "%s: invalid value: %s",
                command->cmd.str,
                command->args[0].str);
    return NULL;
  }

  *dest = value;
  return command->next;
}

static pl2b_Cmd *configBoolAttr(pl2b_Program *program,
                                _Bool *dest,
                                pl2b_Cmd *command,
                                Error *error) {
  (void)program;

  if (pl2b_argsLen(command) != 1) {
    formatError(error, command->sourceInfo, -1,
                "%s: expects exactly one argument",
                command->cmd.str);
    return NULL;
  }

  if (strcmp_icase(command->args[0].str, "true")
      || !strcmp(command->args[0].str, "1")) {
    *dest = 1;
  } else if (strcmp_icase(command->args[0].str, "false")
             || !strcmp(command->args[0].str, "0")) {
    *dest = 0;
  } else {
    formatError(error, command->sourceInfo, -1,
                "%s: expects 'true', 'false', '1' or '0'",
                command->cmd.str);
    return NULL;
  }

  return command->next;
}

static pl2b_Cmd *configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program,
                       &config->port,
                       command,
                       error,
                       0,
                       65536);
}

static pl2b_Cmd *configPend(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program,
                       &config->maxPending,
                       command,
                       error,
                       INT_MIN,
                       INT_MIN);
}

static pl2b_Cmd *configPreloadDyn(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error) {
  Config *config = (Config*)context;
  return configBoolAttr(program,
                        &config->preloadDynamic,
                        command,
                        error);
}

static pl2b_Cmd *configIgnoreCase(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error) {
  Config *config = (Config*)context;
  return configBoolAttr(program,
                        &config->ignoreCase,
                        command,
                        error);
}

static pl2b_Cmd *configCacheTime(pl2b_Program *program,
                                 void *context,
                                 pl2b_Cmd *command,
                                 Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program,
                       &config->cacheTime,
                       command,
                       error,
                       -1,
                       31536000);
}

static pl2b_Cmd* addRoute(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          Error *error) {
  (void)program;

  Config *config = (Config*)context;
  const char *methodStr = command->cmd.str;

  if (pl2b_argsLen(command) != 3) {
    formatError(error, command->sourceInfo, -1,
                "%s: expect exactly three arguments",
                methodStr);
    return NULL;
  }

  HttpMethod method;
  if (!strcmp(methodStr, "GET")
      || !strcmp(methodStr, "Get")
      || !strcmp(methodStr, "get")) {
    method = HTTP_GET;
  } else if (!strcmp(methodStr, "POST")
             || !strcmp(methodStr, "Post")
             || !strcmp(methodStr, "post")) {
    method = HTTP_POST;
  } else {
    assert(0 && "unreachable");
  }

  HandlerType handlerType;
  const char *handlerTypeStr = command->args[1].str;
  if (strcmp_icase(handlerTypeStr,
                   HANDLER_TYPE_NAMES[HDLR_STATIC])) {
    handlerType = HDLR_STATIC;
  } else if (strcmp_icase(handlerTypeStr,
                          HANDLER_TYPE_NAMES[HDLR_DCGI])) {
    handlerType = HDLR_DCGI;
  } else if (strcmp_icase(handlerTypeStr,
                          HANDLER_TYPE_NAMES[HDLR_INTERN])) {
    handlerType = HDLR_INTERN;
  } else if (strcmp_icase(handlerTypeStr,
                          HANDLER_TYPE_NAMES[HDLR_DIR])) {
    handlerType = HDLR_DIR;
  } else {
    formatError(error, command->sourceInfo, -1,
                "%s: incorrect handler type: %s",
                   methodStr, handlerTypeStr);
    return NULL;
  }

  const char *path = command->args[0].str;
  size_t routeCount = ccVecSize(&config->routes);
  for (size_t i = 0; i < routeCount; i++) {
    Route *route = (Route*)ccVecNth(&config->routes, i);
    if (!strcmp(route->path, path) && route->httpMethod == method) {
      formatError(error, command->sourceInfo, -1,
                  "%s: handler for path \"%s\" already exists",
                  methodStr, path);
      return NULL;
    }
  }

  const char *handler = command->args[2].str;

  Route *route = (Route*)malloc(sizeof(Route));
  route->httpMethod = method;
  route->path = path;
  route->handlerType = handlerType;
  route->handlerPath = handler;

  if (route->handlerType == HDLR_DCGI && config->preloadDynamic != 0) {
    LOG_DBG("preloading dynamic library \"%s\"", route->handlerPath);
    route->extra = loadDCGIModule(route->handlerPath, error);
    if (isError(error)) {
      return NULL;
    }
  } else {
    route->extra = NULL;
  }

  ccVecPushBack(&config->routes, (void*)route);

  return command->next;
}

