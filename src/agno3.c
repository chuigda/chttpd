#include "agno3.h"
#include "cc_list.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct st_kv_like {
  const char *key;
} KVLike;

typedef struct st_var {
  const char *varName;
  Value value;
} Var;

typedef struct st_function {
  const char *funcName;
  pl2b_Cmd *funcCmd;
} Function;

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

typedef struct st_flask_impl {
  HttpRequest request;

  int code;
  const char *statusText;
  ccVec TP(StringPair) headers;

  ccList TP(SNDoc) docChain;
  ccList TP(ScopeBase) scopeChain;

  GlobalScope *globalScope;
  ScopeBase *curScope;
  VarContextBase *curVarContext;
  HtmlDoc *rootDoc;
  HtmlDoc *curDoc;
} FlaskImpl;
