#include "agno3.h"
#include "cc_list.h"
#include "html.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------- values and vars ------------------------ */

typedef enum e_value_type {
  VT_NUMBER, VT_STRING, VT_LIST, VT_DICT, VT_NULL
} ValueType;

typedef struct st_value {
  ValueType valueType;
  union {
    int32_t ivalue;
    char *svalue;
    ccVec TP(Value) lsvalue;
    ccVec TP(Var) dict;
  } data;
} Value;

static Value createIntValue(int32_t ivalue);
static Value createStrValue(char *svalue);
static Value createListValue(ccVec TP(Value) lsvalue);
static Value createDictValue(ccVec TP(Var) dict);
static Value createNullValue(void);
static Value copyValue(Value src);
static void dropValue(Value value);

static Value lookupDict(Value dict, const char *key);
static Value listNth(Value list, int index);

typedef struct st_kv_like {
  const char *key;
} KVLike;

typedef struct st_var {
  const char *varName;
  Value value;
} Var;

static Var createIntVar(const char *varName, int32_t ivalue);
static Var createStrVar(const char *varName, char *svalue);
static Var createListVar(const char *varName, ccVec TP(Value) lsvalue);
static Var createDictVar(const char *varName, ccVec TP(Var) dict);
static void varSetInt(Var var, int32_t ivalue);
static void varSetStr(Var var, char *svalue);
static void varSetList(Var var, ccVec TP(Value) lsvalue);
static void varSetDict(Var var, ccVec TP(Value) dict);
static void dropVar(Var var);

static Value createIntValue(int32_t ivalue) {
  return (Value) {
    .valueType = VT_NUMBER, .data = { .ivalue = ivalue }
  };
}

static Value createStrValue(char *svalue) {
  return (Value) {
    .valueType = VT_STRING, .data = { .svalue = svalue }
  };
}

static Value createListValue(ccVec TP(Value) lsvalue) {
  return (Value) {
    .valueType = VT_LIST, .data = { .lsvalue = lsvalue }
  };
}

static Value createDictValue(ccVec TP(Var) dict) {
  return (Value) {
    .valueType = VT_DICT, .data = { .dict = dict }
  };
}

static Value createNullValue(void) {
  return (Value) {
    .valueType = VT_NULL, .data = { .ivalue = 0 }
  };
}

static void dropValue(Value value) {
  switch (value.valueType) {
    case VT_NUMBER:
      break;
    case VT_STRING:
      free(value.data.svalue);
      break;
    case VT_LIST:
      for (size_t i = 0; i < ccVecLen(&value.data.lsvalue); i++) {
        Value *v = (Value*)ccVecNth(&value.data.lsvalue, i);
        dropValue(*v);
      }
      break;
    case VT_DICT:
      for (size_t i = 0; i < ccVecLen(&value.data.dict); i++) {
        Var *var = (Var*)ccVecNth(&value.data.dict, i);
        dropValue(var->value);
      }
      break;
    case VT_NULL:
      break;
  }
}

static Var createIntVar(const char *varName, int32_t ivalue) {
  return (Var) { .varName = varName, .value = createIntValue(ivalue) };
}

static Var createStrVar(const char *varName, char *svalue) {
  return (Var) { .varName = varName, .value = createStrValue(svalue) };
}

static void varSetInt(Var var, int32_t ivalue) {
  dropVar(var);
  var.value = createIntValue(ivalue);
}

static void varSetStr(Var var, char *svalue) {
  dropVar(var);
  var.value = createStrValue(svalue);
}

static void dropVar(Var var) {
  dropValue(var.value);
}

static Value lookupDict(Value dict, const char *key) {
  assert(dict.valueType == VT_DICT);

  size_t dictSize = ccVecLen(&dict.data.dict);
  for (size_t i = 0; i < dictSize; i++) {
    Var *dictItem = (Var*)ccVecNth(&dict.data.dict, i);
    if (!strcmp(dictItem->varName, key)) {
      return dictItem->value;
    }
  }
  return createNullValue();
}

static Value listNth(Value list, int index) {
  assert(list.valueType == VT_LIST);

  size_t listSize = ccVecLen(&list.data.lsvalue);
  if (abs(index) > listSize) {
    return createNullValue();
  }
}

typedef struct st_function {
  const char *funcName;
  pl2b_Cmd *funcCmd;
} Function;

typedef enum e_sn_scope_type {
  SN_SCOPE_GLOBAL = 0,
  SN_SCOPE_IF     = 1,
  SN_SCOPE_WHILE  = 2,
  SN_SCOPE_FUNC   = 3,
} SNScopeType;

#define SCOPE_COMMON \
  int scopeType;

typedef struct st_scope_base {
  SCOPE_COMMON
} ScopeBase;

#define VAR_CTX_COMMON \
  SCOPE_COMMON \
  ccVec TP(Var) vars;

typedef struct st_var_context_base {
  VAR_CTX_COMMON
} VarContextBase;

typedef struct st_global_scope {
  VAR_CTX_COMMON
  ccVec TP(Function) funcs;
} GlobalScope;

typedef struct st_if_scope {
  SCOPE_COMMON

  pl2b_Cmd *ifStmt;
  _Bool conditionResult;
} IfScope;

typedef struct st_while_scope {
  SCOPE_COMMON

  pl2b_Cmd *whileStmt;
} WhileScope;

typedef struct st_func_scope {
  VAR_CTX_COMMON
} FuncScope;

static GlobalScope *createGlobalScope(void);

static GlobalScope *createGlobalScope(void) {
  GlobalScope *ret = (GlobalScope*)malloc(sizeof(GlobalScope));

  ret->scopeType = SN_SCOPE_GLOBAL;
  ccVecInit(&ret->vars, sizeof(Var));
  ccVecInit(&ret->funcs, sizeof(Function));
  return ret;
}

typedef struct st_flask_impl {
  HttpRequest request;

  int code;
  const char *statusText;
  ccVec TP(StringPair) headers;

  ccList TP(HtmlDoc*) docChain;
  ccList TP(ScopeBase*) scopeChain;

  GlobalScope *globalScope;
  ScopeBase *curScope;
  VarContextBase *curVarContext;
  HtmlDoc *rootDoc;
  HtmlDoc *curDoc;
} FlaskImpl;

static Value evalVar(FlaskImpl *flask,
                     const char *var,
                     pl2b_Error *error);
static Value evalVarStr(FlaskImpl *flask,
                        const char *varStr,
                        pl2b_Error *error);
static const char* copyVarPart(char *destBuffer,
                               const char *varStr);

static Value evalVar(FlaskImpl *flask,
                     const char *var,
                     pl2b_Error *error) {
  VarContextBase *scope = flask->curVarContext;

  while (scope != NULL) {
    size_t varCount = ccVecSize(&scope->vars);
    for (size_t i = 0; i < varCount; i++) {
      Var *foundVar = (Var*)ccVecNth(&scope->vars, i);
      if (!strcmp(foundVar->varName, var)) {
        return foundVar->value;
      }
    }
  }
  return createNullValue();
}

static Value evalVarStr(FlaskImpl *flask,
                        const char *varStr,
                        pl2b_Error *error) {
  VarContextBase *scope = flask->curVarContext;

  size_t varStrLen = strlen(varStr);
  char buffer[varStrLen + 1];

  const char *afterFirstPart = copyVarPart(buffer, varStr);
  Value rootValue = evalVar(flask, buffer, error);
  if (pl2b_isError(error)) {
    return createNullValue();
  }

  if (*afterFirstPart == '\0') {
    return rootValue;
  }

  while (*afterFirstPart != '\0') {
    afterFirstPart = copyVarPart(buffer, afterFirstPart);
    if (rootValue.valueType == VT_DICT) {
      // TODO
    } else if (rootValue.valueType == VT_LIST) {
      // TODO
    } else {
      return createNullValue();
    }
  }
}

static const char *copyVarPart(char *destBuffer,
                               const char *varStr) {
  assert(varStr[0] != '\0' && varStr[0] != '/');

  while (*varStr != '/' && *varStr != '\0') {
    *destBuffer = *varStr;
    destBuffer++;
    varStr++;
  }

  *destBuffer = '\0';
  return varStr;
}

Flask createFlask(HttpRequest request) {
  FlaskImpl *flaskImpl = (FlaskImpl*)malloc(sizeof(FlaskImpl));
  flaskImpl->request = request;

  flaskImpl->code = -1;
  flaskImpl->statusText = NULL;

  ccVecInit(&flaskImpl->headers, sizeof(StringPair));

  ccListInit(&flaskImpl->docChain, sizeof(HtmlDoc*));
  ccListInit(&flaskImpl->scopeChain, sizeof(ScopeBase*));

  flaskImpl->globalScope = createGlobalScope();
  flaskImpl->curScope = (ScopeBase*)flaskImpl->globalScope;
  flaskImpl->curVarContext = (VarContextBase*)flaskImpl->globalScope;

  flaskImpl->rootDoc = htmlRootDoc();
  flaskImpl->curDoc = flaskImpl->rootDoc;

  ccListPushBack(&flaskImpl->scopeChain, &flaskImpl->curScope);
  ccListPushBack(&flaskImpl->docChain, &flaskImpl->rootDoc);

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
  const char *valueStr = command->args[1].str;

  if (stricmp(keyStr, "Content-Length")) {
    pl2b_errPrintf(error, PL2B_ERR_USER, command->sourceInfo, NULL,
                   "header: cannot set `Content-Length` manually");
    return NULL;
  }

  char *key;
  char *value;

  // TODO
  /*
  key = keyStr[0] == '$'
    ? evalVarStr(flask, keyStr + 1, error)
    : copyString(keyStr);
  if (pl2b_isError(error)) {
    error->sourceInfo = command->sourceInfo;
    return NULL;
  }
  */

  // TODO
  /*
  value = valueStr[0] == '$'
    ? evalVarStr(flask, valueStr + 1, error)
    : copyString(valueStr);
  if (pl2b_isError(error)) {
    error->sourceInfo = command->sourceInfo;
    return NULL;
  }
  */

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
