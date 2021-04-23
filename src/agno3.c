#include "agno3.h"
#include "cc_list.h"
#include "html.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "agno3_impl.c"

typedef struct st_flask_impl {
  ScriptingContext scriptingContext;
  
  HttpRequest request;

  int code;
  const char *statusText;
  ccVec TP(StringPair) headers;
  ccVec TP(HtmlDoc*) docChain;
  HtmlDoc *rootDoc;
  HtmlDoc *curDoc;
} FlaskImpl;

Flask createFlask(HttpRequest request) {
  FlaskImpl *flaskImpl = (FlaskImpl*)malloc(sizeof(FlaskImpl));
  
  initScriptingContext(&flaskImpl->scriptingContext);
  
  flaskImpl->request = request;
  flaskImpl->code = -1;
  flaskImpl->statusText = NULL;

  ccVecInit(&flaskImpl->headers, sizeof(StringPair));
  ccVecInit(&flaskImpl->docChain, sizeof(HtmlDoc*));

  flaskImpl->rootDoc = htmlRootDoc();
  flaskImpl->curDoc = flaskImpl->rootDoc;
  ccVecPushBack(&flaskImpl->docChain, &flaskImpl->rootDoc);

  return (Flask)flaskImpl;
}

static pl2b_Cmd* setStatus(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error);
static pl2b_Cmd* setHeader(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error);

static pl2b_Cmd *handleScript(pl2b_Program *program,
                              void *context,
                              pl2b_Cmd *command,
                              pl2b_Error *error);
static _Bool routeOpenTag(pl2b_CmdPart cmdPart);
static _Bool routeCloseTag(pl2b_CmdPart cmdPart);
static pl2b_Cmd *handleOpenTag(pl2b_Program *program,
                               void *context,
                               pl2b_Cmd *command,
                               pl2b_Error *error);
static pl2b_Cmd *handleCloseTag(pl2b_Program *program,
                                void *context,
                                pl2b_Cmd *command,
                                pl2b_Error *error);

static pl2b_Cmd *handleSet(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error);
static pl2b_Cmd *handleCall(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error);
static pl2b_Cmd *handleIf(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          pl2b_Error *error);
static pl2b_Cmd *handleWhile(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error);
static pl2b_Cmd *handleBreak(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error);
static pl2b_Cmd *handleProc(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error);
static pl2b_Cmd *handleEnd(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error);
static pl2b_Cmd *handleAbort(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error);

static pl2b_Cmd *maybeHandleCustomTag(pl2b_Program *program,
                                      void *context,
                                      pl2b_Cmd *command,
                                      pl2b_Error *error);

const pl2b_Language *getAgNO3(void) {
  static pl2b_PCallCmd cmds[] = {
    // HTTP operation
    { "status", NULL,           setStatus,      0, 0 },
    { "header", NULL,           setHeader,      0, 0 },

    // HTML operation
    { "script", NULL,           handleScript,   0, 0 },
    { NULL,     routeOpenTag,   handleOpenTag,  0, 0 },
    { NULL,     routeCloseTag,  handleCloseTag, 0, 0 },

    // AgNO3 Language operation
    { "set",    NULL,           handleSet,      0, 0 },
    { "call",   NULL,           handleCall,     0, 0 },
    { "if",     NULL,           handleIf,       0, 0 },
    { "while",  NULL,           handleWhile,    0, 0 },
    { "break",  NULL,           handleBreak,    0, 0 },
    { "proc",   NULL,           handleProc,     0, 0 },
    { "end",    NULL,           handleEnd,      0, 0 },
    { "abort",  NULL,           handleAbort,    0, 0 },

    { NULL,     NULL,           NULL,           0, 0 }
  };

  static pl2b_Language language = (pl2b_Language) {
    "AgNO3 HTML DSL",
    "AgNO3 is a toy DSL for generating HTML document",
    cmds,
    maybeHandleCustomTag
  };

  return &language;
}

static pl2b_Cmd* setStatus(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error) {
  (void)program;

  if (pl2b_argsLen(command) != 2) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "status: expecting 2 arguments");
    return NULL;
  }

  FlaskImpl *flask = (FlaskImpl*)context;

  const char *statusCodeStr = command->args[0].str;
  const char *statusText = command->args[1].str;

  int statusCode = atoi(statusCodeStr);
  if (statusCode == 0) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo,
                   "status: invalid status code: %s",
                   statusCodeStr);
    return NULL;
  }

  flask->code = statusCode;
  flask->statusText = statusText;

  return command->next;
}

static pl2b_Cmd* setHeader(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error) {
  (void)program;

  if (pl2b_argsLen(command) != 2) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "header: expecting 2 arguments");
    return NULL;
  }

  FlaskImpl *flask = (FlaskImpl*)context;

  const char *keyStr = command->args[0].str;
  // const char *valueStr = command->args[1].str;

  if (stricmp(keyStr, "Content-Length")) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "header: cannot set `Content-Length` manually");
    return NULL;
  }

  char *key = ""; // TODO
  char *value = ""; // TODO

  for (size_t i = 0; i < ccVecLen(&flask->headers); i++) {
    StringPair *pair = (StringPair*)ccVecNth(&flask->headers, i);
    if (stricmp(pair->first, key)) {
      pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                     "header: duplicate header: %s", key);
      free(key);
      free(value);
      return NULL;
    }
  }

  StringPair p = (StringPair) { key, value };
  ccVecPushBack(&flask->headers, &p);

  return command->next;
}

static pl2b_Cmd *handleScript(pl2b_Program *program,
                              void *context,
                              pl2b_Cmd *command,
                              pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static _Bool routeOpenTag(pl2b_CmdPart cmdPart)  {
  // TODO
  (void)cmdPart;
  return 0;
}

static _Bool routeCloseTag(pl2b_CmdPart cmdPart) {
  // TODO
  (void)cmdPart;
  return 0;
}

static pl2b_Cmd *handleOpenTag(pl2b_Program *program,
                               void *context,
                               pl2b_Cmd *command,
                               pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleCloseTag(pl2b_Program *program,
                                void *context,
                                pl2b_Cmd *command,
                                pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleSet(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleCall(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleIf(pl2b_Program *program,
                          void *context,
                          pl2b_Cmd *command,
                          pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleWhile(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleBreak(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleProc(pl2b_Program *program,
                            void *context,
                            pl2b_Cmd *command,
                            pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleEnd(pl2b_Program *program,
                           void *context,
                           pl2b_Cmd *command,
                           pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *handleAbort(pl2b_Program *program,
                             void *context,
                             pl2b_Cmd *command,
                             pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}

static pl2b_Cmd *maybeHandleCustomTag(pl2b_Program *program,
                                      void *context,
                                      pl2b_Cmd *command,
                                      pl2b_Error *error) {
  // TODO
  (void)program;
  (void)context;
  (void)command;
  (void)error;
  return NULL;
}
