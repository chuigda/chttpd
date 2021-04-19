#include "agno3.h"

typedef enum e_value_type {
  VT_NUMBER, VT_STRING, VT_LIST, VT_DICT, VT_NULL
} ValueType;

typedef struct st_value {
  ValueType valueType;
  union {
    int32_t ivalue;
    char *svalue;
    ccVec TP(Value) lsvalue;
    ccVec TP(DictItem) dict;
  } data;
} Value;

typedef struct st_dict_item {
  char *key;
  Value value;
} DictItem;

static Value createIntValue(int32_t ivalue);
static Value createStrValue(char *svalue);
static Value createListValue(ccVec TP(Value) lsvalue);
static Value createDictValue(ccVec TP(DictItem) dict);
static Value createNullValue(void);
static Value copyValue(Value src);
static void dropValue(Value value);

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
        DictItem *dictItem = (DictItem*)ccVecNth(&value.data.dict, i);
        free(dictItem->key);
        dropValue(dictItem->value);
      }
      break;
    case VT_NULL:
      break;
  }
}

/* interface KVLike */
typedef struct st_kv_like {
  const char *key;
} KVLike;

/* class Var implementes KVLike */
typedef struct st_var {
  const char *varName;
  Value value;
} Var;

/* class Function implements KVLike */
typedef struct st_function {
  const char *funcName;
  pl2b_Cmd *funcCmd;
} Function;

#define VAR_CTX_MASK 0b1000

typedef enum e_sn_scope_type {
  SN_SCOPE_GLOBAL = 0b1001,
  SN_SCOPE_FUNC   = 0b1010,
  SN_SCOPE_IF     = 0b0001,
  SN_SCOPE_WHILE  = 0b0010,
} SNScopeType;

#define SCOPE_COMMON \
  int scopeType;

/* abstract class Scope */
typedef struct st_scope {
  SCOPE_COMMON
} Scope;

#define VAR_CTX_COMMON \
  SCOPE_COMMON \
  ccVec TP(Var) vars;

/* abstract class VarContext extends Scope */
typedef struct st_var_context {
  VAR_CTX_COMMON
} VarContext;

/* class GlobalScope extends VarContext */
typedef struct st_global_scope {
  VAR_CTX_COMMON
  ccVec TP(Function) funcs;
} GlobalScope;

/* class FunctionScope extends VarContext */
typedef struct st_func_scope {
  VAR_CTX_COMMON
} FuncScope;

/* class IfScope extends Scope */
typedef struct st_if_scope {
  SCOPE_COMMON
  pl2b_Cmd *ifStmt;
  _Bool conditionResult;
} IfScope;

/* class WhileScope extends Scope */
typedef struct st_while_scope {
  SCOPE_COMMON
  pl2b_Cmd *whileStmt;
} WhileScope;

static GlobalScope *createGlobalScope(void);

static GlobalScope *createGlobalScope(void) {
  GlobalScope *ret = (GlobalScope*)malloc(sizeof(GlobalScope));

  ret->scopeType = SN_SCOPE_GLOBAL;
  ccVecInit(&ret->vars, sizeof(Var));
  ccVecInit(&ret->funcs, sizeof(Function));
  return ret;
}

typedef struct st_scripting_context {
  ccVec TP(Scope*) scopeChain;
  ccVec TP(VarContext*) varContextChain;
  GlobalScope *globalScope;
  Scope *currentScope;
  VarContext *currentVarContext;
} ScriptingContext;

static void initScriptingContext(ScriptingContext *context) {
  ccVecInit(&context->scopeChain, sizeof(Scope*));
  ccVecInit(&context->varContextChain, sizeof(VarContext*));
  
  context->globalScope = createGlobalScope();
  context->currentScope = (Scope*)context->globalScope;
  context->currentVarContext = (VarContext*)context->globalScope;
  
  ccVecPushBack(&context->scopeChain, &context->globalScope);
  ccVecPushBack(&context->varContextChain, &context->globalScope);
}

static void dropScriptingContext(ScriptingContext *context) {
  (void)context;
  // TODO
}
