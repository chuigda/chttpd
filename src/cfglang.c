#include "cfglang.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ADDRESS     "127.0.0.1"
#define DEFAULT_PORT        8080
#define DEFAULT_MAX_PENDING 16
#define DEFAULT_CGI_TIMEOUT 5

const char *HANDLER_TYPE_NAMES[] = {
  [HDLR_SCRIPT] = "SCRIPT",
  [HDLR_CGI] = "CGI",
  [HDLR_STATIC] = "STATIC"
};

void initConfig(Config *config) {
  config->address = DEFAULT_ADDRESS;
  config->port = DEFAULT_PORT;
  config->maxPending = DEFAULT_MAX_PENDING;
  config->cgiTimeout = DEFAULT_CGI_TIMEOUT;
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
                               Error *error);

static pl2b_Cmd* configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error);

static pl2b_Cmd *configPend(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error);

static pl2b_Cmd *configCGITimeout(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error);

static pl2b_Cmd* addRoute(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          Error *error);

const pl2b_Language *getCfgLanguage(void) {
  static pl2b_PCallCmd pCallCmds[] = {
    { "listen-address", NULL, configAddr,       0, 0 },
    { "listen-port",    NULL, configPort,       0, 0 },
    { "max-pending",    NULL, configPend,       0, 0 },
    { "cgi-timeout",    NULL, configCGITimeout, 0, 0 },
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
                               Error *error) {
  (void)program;

  if (pl2b_argsLen(command) != 1) {
    formatError(error, command->sourceInfo, -1,
                "listen-port: expects exactly one argument");
    return NULL;
  }

  int value = atoi(command->args[0].str);
  if (value <= 0 || value >= 65536) {
    formatError(error, command->sourceInfo, -1,
                "%s: invalid port: %s",
                command->cmd, command->args[0].str);
    return NULL;
  }

  *dest = value;
  return command->next;
}

static pl2b_Cmd *configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program, &config->port, command, error);
}

static pl2b_Cmd *configPend(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program, &config->maxPending, command, error);
}

static pl2b_Cmd *configCGITimeout(pl2b_Program *program,
                                  void *context,
                                  pl2b_Cmd *command,
                                  Error *error) {
  Config *config = (Config*)context;
  return configIntAttr(program, &config->cgiTimeout, command, error);
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
  if (!strcmp(handlerTypeStr, "SCRIPT")
      || !strcmp(handlerTypeStr, "script")
      || !strcmp(handlerTypeStr, "Script")) {
    handlerType = HDLR_SCRIPT;
  } else if (!strcmp(handlerTypeStr, "CGI")
             || !strcmp(handlerTypeStr, "cgi")
             || !strcmp(handlerTypeStr, "Cgi")) {
    handlerType = HDLR_CGI;
  } else if (!strcmp(handlerTypeStr, "STATIC")
             || !strcmp(handlerTypeStr, "static")
             || !strcmp(handlerTypeStr, "Static")) {
    handlerType = HDLR_STATIC;
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
    if (!strcmp(route->path, path)) {
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

  ccVecPushBack(&config->routes, (void*)route);

  return command->next;
}
