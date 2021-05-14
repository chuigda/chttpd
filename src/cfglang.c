#include "cfglang.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *HANDLER_TYPE_NAMES[] = {
  [HDLR_SCRIPT] = "SCRIPT",
  [HDLR_CGI] = "CGI",
  [HDLR_STATIC] = "STATIC"
};

void initConfig(Config *config) {
  config->address = NULL;
  config->port = 0;
  ccVecInit(&config->routes, sizeof(Route));
}

void dropConfig(Config *config) {
  ccVecDestroy(&config->routes);
}

static pl2b_Cmd* configAddr(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error);

static pl2b_Cmd* configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error);

static pl2b_Cmd* addRoute(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          pl2b_Error *error);

const pl2b_Language *getCfgLanguage(void) {
  static pl2b_PCallCmd pCallCmds[] = {
    { "listen-address", NULL, configAddr, 0, 0 },
    { "listen-port",    NULL, configPort, 0, 0 },
    { "post",           NULL, addRoute,   0, 0 },
    { "POST",           NULL, addRoute,   0, 0 },
    { "Post",           NULL, addRoute,   0, 0 },
    { "get",            NULL, addRoute,   0, 0 },
    { "Get",            NULL, addRoute,   0, 0 },
    { "GET",            NULL, addRoute,   0, 0 },
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
                            pl2b_Error *error) {
  (void)program;

  Config *config = (Config*)context;
  if (config->address != NULL) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "server address already set to: %s",
                   config->address);
    return NULL;
  }

  if (pl2b_argsLen(command) != 1) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "listen-address: expects exactly one argument");
    return NULL;
  }

  config->address = command->args[0].str;
  return command->next;
}

static pl2b_Cmd *configPort(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error) {
  (void)program;

  Config *config = (Config*)context;
  if (config->port != 0) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "server port already set to: %d",
                   config->port);

  }

  if (pl2b_argsLen(command) != 1) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "listen-port: expects exactly one argument");
    return NULL;
  }

  int port = atoi(command->args[0].str);
  if (port <= 0 || port >= 65536) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "listen-port: invalid port: %s",
                   command->args[0].str);
    return NULL;
  }

  config->port = port;
  return command->next;
}

static pl2b_Cmd* addRoute(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          pl2b_Error *error) {
  (void)program;

  Config *config = (Config*)context;
  const char *methodStr = command->cmd.str;

  if (pl2b_argsLen(command) != 3) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
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
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "%s: incorrect handler type: %s",
                   methodStr, handlerTypeStr);
    return NULL;
  }

  const char *path = command->args[0].str;
  size_t routeCount = ccVecSize(&config->routes);
  for (size_t i = 0; i < routeCount; i++) {
    Route *route = (Route*)ccVecNth(&config->routes, i);
    if (!strcmp(route->path, path)) {
      pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
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
