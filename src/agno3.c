#include "agno3.h"
#include "cc_list.h"
#include "html.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef enum e_value_type {
  Number,
  String
} ValueType;

typedef struct st_value {
  ValueType valueType;
  union {
    int32_t ivalue;
    char *svalue;
  } data;
} Value;

static Value createIntValue(int32_t ivalue);
static Value createStrValue(char *svalue);
static void dropValue(Value value);

static Value createIntValue(int32_t ivalue) {
  return (Value) { .valueType = Number, .data = { .ivalue = ivalue } };
}

static Value createStrValue(char *svalue) {
  return (Value) { .valueType = String, .data = { .svalue = svalue } };
}

static void dropValue(Value value) {
  if (value.valueType == String) {
    free(value.data.svalue);
  }
}

typedef struct st_kv_like {
  const char *key;
} KVLike;

typedef struct st_var {
  const char *varName;
  Value value;
} Var;

static Var createIntVar(const char *varName, int32_t ivalue);
static Var createStrVar(const char *varName, char *svalue);
static void varSetInt(Var var, int32_t ivalue);
static void varSetStr(Var var, char *svalue);
static void dropVar(Var var);

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
