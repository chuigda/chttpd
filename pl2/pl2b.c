#include "pl2b.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*** ----------------- Transmute any char to uchar ----------------- ***/

static unsigned char transmuteU8(char i8) {
  return *(unsigned char*)(&i8);
}

/*** ------------------- Implementation of slice ------------------- ***/

typedef struct st_slice {
  char *start;
  char *end;
} Slice;

static Slice slice(char *start, char *end);
static Slice nullSlice(void);
static char *sliceIntoCStr(Slice slice);
static _Bool isNullSlice(Slice slice);

Slice slice(char *start, char *end) {
  Slice ret;
  if (start == end) {
    ret.start = NULL;
    ret.end = NULL;
  } else {
    ret.start = start;
    ret.end = end;
  }
  return ret;
}

static Slice nullSlice(void) {
  Slice ret;
  ret.start = NULL;
  ret.end = NULL;
  return ret;
}

static char *sliceIntoCStr(Slice slice) {
  if (isNullSlice(slice)) {
    return NULL;
  }
  if (*slice.end != '\0') {
    *slice.end = '\0';
  }
  return slice.start;
}

static _Bool isNullSlice(Slice slice) {
  return slice.start == slice.end;
}

/*** ----------------- Implementation of pl2b_Error ---------------- ***/

pl2b_Error *pl2b_errorBuffer(uint16_t strBufferSize) {
  pl2b_Error *ret = (pl2b_Error*)malloc(sizeof(pl2b_Error) + strBufferSize);
  if (ret == NULL) {
    return NULL;
  }
  memset(ret, 0, sizeof(pl2b_Error) + strBufferSize);
  ret->errorBufferSize = strBufferSize;
  return ret;
}

void pl2b_errPrintf(pl2b_Error *error,
                    uint16_t errorCode,
                    pl2b_SourceInfo sourceInfo,
                    void *extraData,
                    const char *fmt,
                    ...) {
  error->errorCode = errorCode;
  error->extraData = extraData;
  error->sourceInfo = sourceInfo;
  if (error->errorBufferSize == 0) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(error->reason, error->errorBufferSize, fmt, ap);
  va_end(ap);
}

void pl2b_dropError(pl2b_Error *error) {
  if (error->extraData) {
    free(error->extraData);
  }
  free(error);
}

_Bool pl2b_isError(pl2b_Error *error) {
  return error->errorCode != 0;
}

/*** ------------------- Some toolkit functions -------------------- ***/

pl2b_SourceInfo pl2b_sourceInfo(const char *fileName, uint16_t line) {
  pl2b_SourceInfo ret;
  ret.fileName = fileName;
  ret.line = line;
  return ret;
}

pl2b_CmdPart pl2b_cmdPart(char *str, _Bool isString) {
  return (pl2b_CmdPart) { str, isString };
}

pl2b_Cmd *pl2b_cmd3(pl2b_SourceInfo sourceInfo,
                    pl2b_CmdPart cmd,
                    pl2b_CmdPart args[]) {
  return pl2b_cmd5(NULL, NULL, sourceInfo, cmd, args);
}

pl2b_Cmd *pl2b_cmd5(pl2b_Cmd *prev,
                    pl2b_Cmd *next,
                    pl2b_SourceInfo sourceInfo,
                    pl2b_CmdPart cmd,
                    pl2b_CmdPart args[]) {
  uint16_t argLen = 0;
  for (; !PL2B_EMPTY_PART(args[argLen]); ++argLen);

  pl2b_Cmd *ret = (pl2b_Cmd*)malloc(sizeof(pl2b_Cmd) +
                                    (argLen + 1) * sizeof(pl2b_CmdPart));
  if (ret == NULL) {
    return NULL;
  }
  ret->prev = prev;
  if (prev != NULL) {
    prev->next = ret;
  }
  ret->next = next;
  if (next != NULL) {
    next->prev = ret;
  }
  ret->sourceInfo = sourceInfo;
  ret->cmd = cmd;
  for (uint16_t i = 0; i < argLen; i++) {
    ret->args[i] = args[i];
  }
  ret->args[argLen] = pl2b_cmdPart(NULL, 0);
  return ret;
}

uint16_t pl2b_argsLen(pl2b_Cmd *cmd) {
  uint16_t acc = 0;
  pl2b_CmdPart *iter = cmd->args;
  while (!PL2B_EMPTY_PART(*iter)) {
    iter++;
    acc++;
  }
  return acc;
}

/*** ---------------- Implementation of pl2b_Program --------------- ***/

void pl2b_initProgram(pl2b_Program *program) {
  program->commands = NULL;
}

void pl2b_dropProgram(pl2b_Program *program) {
  pl2b_Cmd *iter = program->commands;
  while (iter != NULL) {
    pl2b_Cmd *next = iter->next;
    free(iter);
    iter = next;
  }
}

/*** ----------------- Implementation of pl2b_parse ---------------- ***/

typedef enum e_parse_mode {
  PARSE_SINGLE_LINE = 1,
  PARSE_MULTI_LINE  = 2
} ParseMode;

typedef enum e_ques_cmd {
  QUES_INVALID = 0,
  QUES_BEGIN   = 1,
  QUES_END     = 2
} QuesCmd;

typedef struct st_parsed_part_cache {
  Slice slice;
  _Bool isString;
} ParsedPartCache;

typedef struct st_parse_context {
  pl2b_Program program;
  pl2b_Cmd *listTail;

  char *src;
  uint32_t srcIdx;
  ParseMode mode;

  pl2b_SourceInfo sourceInfo;

  uint32_t parseBufferSize;
  uint32_t parseBufferUsage;
  ParsedPartCache parseBuffer[0];
} ParseContext;

static ParseContext *createParseContext(char *src,
                                        uint16_t parseBufferSize);
static void parseLine(ParseContext *ctx, pl2b_Error *error);
static void parseQuesMark(ParseContext *ctx, pl2b_Error *error);
static void parsePart(ParseContext *ctx, pl2b_Error *error);
static Slice parseId(ParseContext *ctx, pl2b_Error *error);
static Slice parseStr(ParseContext *ctx, pl2b_Error *error);
static void checkBufferSize(ParseContext *ctx, pl2b_Error *error);
static void finishLine(ParseContext *ctx, pl2b_Error *error);
static pl2b_Cmd *cmdFromSlices2(pl2b_SourceInfo sourceInfo,
                                ParsedPartCache *parts);
static pl2b_Cmd *cmdFromSlices4(pl2b_Cmd *prev,
                                pl2b_Cmd *next,
                                pl2b_SourceInfo sourceInfo,
                                ParsedPartCache *parts);
static void skipWhitespace(ParseContext *ctx);
static void skipComment(ParseContext *ctx);
static char curChar(ParseContext *ctx);
static char *curCharPos(ParseContext *ctx);
static void nextChar(ParseContext *ctx);
static _Bool isIdChar(char ch);
static _Bool isLineEnd(char ch);
static char *shrinkConv(char *start, char *end);

pl2b_Program pl2b_parse(char *source,
                        uint16_t parseBufferSize,
                        pl2b_Error *error) {
  ParseContext *context = createParseContext(source, parseBufferSize);
  if (context == NULL) {
    pl2b_errPrintf(error,
                   PL2B_ERR_MALLOC,
                   (pl2b_SourceInfo) {},
                   NULL,
                   "allocation failure");
    return (pl2b_Program) { NULL };
  }

  while (curChar(context) != '\0') {
    parseLine(context, error);
    if (pl2b_isError(error)) {
      break;
    }
  }

  pl2b_Program ret = context->program;
  free(context);
  return ret;
}

static ParseContext *createParseContext(char *src,
                                        uint16_t parseBufferSize) {
  if (parseBufferSize == 0) {
    return NULL;
  }

  ParseContext *ret = (ParseContext*)malloc(
    sizeof(ParseContext) + parseBufferSize * sizeof(Slice)
  );
  if (ret == NULL) {
    return NULL;
  }

  pl2b_initProgram(&ret->program);
  ret->listTail = NULL;
  ret->src = src;
  ret->srcIdx = 0;
  ret->sourceInfo = pl2b_sourceInfo("<unknown-file>", 1);
  ret->mode = PARSE_SINGLE_LINE;

  ret->parseBufferSize = parseBufferSize;
  ret->parseBufferUsage = 0;
  memset(ret->parseBuffer, 0, parseBufferSize * sizeof(Slice));
  return ret;
}

static void parseLine(ParseContext *ctx, pl2b_Error *error) {
  if (curChar(ctx) == '?') {
    parseQuesMark(ctx, error);
    if (pl2b_isError(error)) {
      return;
    }
  }

  while (1) {
    skipWhitespace(ctx);
    if (curChar(ctx) == '\0' || curChar(ctx) == '\n') {
      if (ctx->mode == PARSE_SINGLE_LINE) {
        finishLine(ctx, error);
      }
      if (ctx->mode == PARSE_MULTI_LINE && curChar(ctx) == '\0') {
        pl2b_errPrintf(error, PL2B_ERR_UNCLOSED_BEGIN, ctx->sourceInfo,
                       NULL, "unclosed `?begin` block");
      }
      if (curChar(ctx) == '\n') {
        nextChar(ctx);
      }
      return;
    } else if (curChar(ctx) == '#') {
      skipComment(ctx);
    } else {
      parsePart(ctx, error);
      if (pl2b_isError(error)) {
        return;
      }
    }
  }
}

static void parseQuesMark(ParseContext *ctx, pl2b_Error *error) {
  assert(curChar(ctx) == '?');
  nextChar(ctx);

  char *start = curCharPos(ctx);
  while (isalnum((int)curChar(ctx))) {
    nextChar(ctx);
  }
  char *end = curCharPos(ctx);
  Slice s = slice(start, end);
  char *cstr = sliceIntoCStr(s);

  if (!strcmp(cstr, "begin")) {
    ctx->mode = PARSE_MULTI_LINE;
  } else if (!strcmp(cstr, "end")) {
    ctx->mode = PARSE_SINGLE_LINE;
    finishLine(ctx, error);
  } else {
    pl2b_errPrintf(error, PL2B_ERR_UNKNOWN_QUES, ctx->sourceInfo,
                   NULL, "unknown question mark operator: `%s`", cstr);
  }
}

static void parsePart(ParseContext *ctx, pl2b_Error *error) {
  Slice slice;
  _Bool isString;
  if (curChar(ctx) == '"' || curChar(ctx) == '\'') {
    slice = parseStr(ctx, error);
    isString = 1;
  } else {
    slice = parseId(ctx, error);
    isString = 0;
  }
  if (pl2b_isError(error)) {
    return;
  }

  checkBufferSize(ctx, error);
  if (pl2b_isError(error)) {
    return;
  }

  ctx->parseBuffer[ctx->parseBufferUsage++] =
    (ParsedPartCache) { slice, isString };
}

static Slice parseId(ParseContext *ctx, pl2b_Error *error) {
  (void)error;
  char *start = curCharPos(ctx);
  while (isIdChar(curChar(ctx))) {
    nextChar(ctx);
  }
  char *end = curCharPos(ctx);
  return slice(start, end);
}

static Slice parseStr(ParseContext *ctx, pl2b_Error *error) {
  assert(curChar(ctx) == '"' || curChar(ctx) == '\'');
  nextChar(ctx);

  char *start = curCharPos(ctx);
  while (curChar(ctx) != '"'
         && curChar(ctx) != '\''
         && !isLineEnd(curChar(ctx))) {
    if (curChar(ctx) == '\\') {
      nextChar(ctx);
      nextChar(ctx);
    } else {
      nextChar(ctx);
    }
  }
  char *end = curCharPos(ctx);
  end = shrinkConv(start, end);

  if (curChar(ctx) == '"' || curChar(ctx) == '\'') {
    nextChar(ctx);
  } else {
    pl2b_errPrintf(error, PL2B_ERR_UNCLOSED_BEGIN, ctx->sourceInfo,
                   NULL, "unclosed string literal");
    return nullSlice();
  }
  return slice(start, end);
}

static void checkBufferSize(ParseContext *ctx, pl2b_Error *error) {
  if (ctx->parseBufferSize <= ctx->parseBufferUsage + 1) {
    pl2b_errPrintf(error, PL2B_ERR_UNCLOSED_BEGIN, ctx->sourceInfo,
                   NULL, "command parts exceed internal parsing buffer");
  }
}

static void finishLine(ParseContext *ctx, pl2b_Error *error) {
  (void)error;

  pl2b_SourceInfo sourceInfo = ctx->sourceInfo;
  nextChar(ctx);
  if (ctx->parseBufferUsage == 0) {
    return;
  }
  if (ctx->listTail == NULL) {
    assert(ctx->program.commands == NULL);
    ctx->program.commands =
      ctx->listTail = cmdFromSlices2(ctx->sourceInfo, ctx->parseBuffer);
  } else {
    ctx->listTail = cmdFromSlices4(ctx->listTail, NULL,
                                   ctx->sourceInfo, ctx->parseBuffer);
  }
  if (ctx->listTail == NULL) {
    pl2b_errPrintf(error, PL2B_ERR_MALLOC, sourceInfo, 0,
                   "failed allocating pl2b_Cmd");
  }
  memset(ctx->parseBuffer, 0, sizeof(Slice) * ctx->parseBufferSize);
  ctx->parseBufferUsage = 0;
}

static pl2b_Cmd *cmdFromSlices2(pl2b_SourceInfo sourceInfo,
                                ParsedPartCache *parts) {
  return cmdFromSlices4(NULL, NULL, sourceInfo, parts);
}

static pl2b_Cmd *cmdFromSlices4(pl2b_Cmd *prev,
                                pl2b_Cmd *next,
                                pl2b_SourceInfo sourceInfo,
                                ParsedPartCache *parts) {
  uint16_t partCount = 0;
  for (; !isNullSlice(parts[partCount].slice); ++partCount);

  pl2b_Cmd *ret = (pl2b_Cmd*)malloc(sizeof(pl2b_Cmd)
                                    + partCount * sizeof(pl2b_CmdPart));
  if (ret == NULL) {
    return NULL;
  }

  ret->prev = prev;
  if (prev != NULL) {
    prev->next = ret;
  }
  ret->next = next;
  if (next != NULL) {
    next->prev = ret;
  }
  ret->sourceInfo = sourceInfo;
  ret->cmd = pl2b_cmdPart(sliceIntoCStr(parts[0].slice),
                          parts[0].isString);
  for (uint16_t i = 1; i < partCount; i++) {
    ret->args[i - 1] = pl2b_cmdPart(sliceIntoCStr(parts[i].slice),
                                    parts[i].isString);
  }
  ret->args[partCount - 1] = pl2b_cmdPart(NULL, 0);
  return ret;
}

static void skipWhitespace(ParseContext *ctx) {
  while (1) {
    switch (curChar(ctx)) {
    case ' ': case '\t': case '\f': case '\v': case '\r':
      nextChar(ctx);
      break;
    default:
      return;
    }
  }
}

static void skipComment(ParseContext *ctx) {
  assert(curChar(ctx) == '#');
  nextChar(ctx);

  while (!isLineEnd(curChar(ctx))) {
    nextChar(ctx);
  }

  if (curChar(ctx) == '\n') {
    nextChar(ctx);
  }
}

static char curChar(ParseContext *ctx) {
  return ctx->src[ctx->srcIdx];
}

static char *curCharPos(ParseContext *ctx) {
  return ctx->src + ctx->srcIdx;
}

static void nextChar(ParseContext *ctx) {
  if (ctx->src[ctx->srcIdx] == '\0') {
    return;
  } else {
    if (ctx->src[ctx->srcIdx] == '\n') {
      ctx->sourceInfo.line += 1;
    }
    ctx->srcIdx += 1;
  }
}

static _Bool isIdChar(char ch) {
  unsigned char uch = transmuteU8(ch);
  if (uch >= 128) {
    return 1;
  } else if (isalnum(ch)) {
    return 1;
  } else {
    switch (ch) {
      case '!': case '$': case '%': case '^': case '&': case '*':
      case '(': case ')': case '-': case '+': case '_': case '=':
      case '[': case ']': case '{': case '}': case '|': case '\\':
      case ':': case ';': case '\'': case ',': case '<': case '>':
      case '/': case '?': case '~': case '@': case '.':
        return 1;
      default:
        return 0;
    }
  }
}

static _Bool isLineEnd(char ch) {
  return ch == '\0' || ch == '\n';
}

static char *shrinkConv(char *start, char *end) {
  char *iter1 = start, *iter2 = start;
  while (iter1 != end) {
    if (iter1[0] == '\\') {
      switch (iter1[1]) {
      case 'n': *iter2++ = '\n'; iter1 += 2; break;
      case 'r': *iter2++ = '\r'; iter1 += 2; break;
      case 'f': *iter2++ = '\f'; iter1 += 2; break;
      case 'v': *iter2++ = '\v'; iter1 += 2; break;
      case 't': *iter2++ = '\t'; iter1 += 2; break;
      case 'a': *iter2++ = '\a'; iter1 += 2; break;
      case '"': *iter2++ = '\"'; iter1 += 2; break;
      case '0': *iter2++ = '\0'; iter1 += 2; break;
      default:
        *iter2++ = *iter1++;
      }
    } else {
      *iter2++ = *iter1++;
    }
  }
  return iter2;
}

/*** ----------------------------- Run ----------------------------- ***/

typedef struct st_run_context {
  pl2b_Program *program;
  pl2b_Cmd *curCmd;
  void *userContext;

  const pl2b_Language *language;
} RunContext;

static RunContext *createRunContext(pl2b_Program *program);
static void destroyRunContext(RunContext *context);
static _Bool cmdHandler(RunContext *context,
                        pl2b_Cmd *cmd,
                        pl2b_Error *error);
static _Bool checkNextCmdRet(RunContext *context,
                             pl2b_Cmd *nextCmd,
                             pl2b_Error *error);

void pl2b_run(pl2b_Program *program, pl2b_Error *error) {
  RunContext *context = createRunContext(program);
  if (context == NULL) {
    pl2b_errPrintf(error, PL2B_ERR_MALLOC, pl2b_sourceInfo(NULL, 0),
                   NULL, "run: cannot allocate memory for run context");
    return;
  }

  while (cmdHandler(context, context->curCmd, error)) {
    if (pl2b_isError(error)) {
      break;
    }
  }

  destroyRunContext(context);
}

void pl2b_runWithLanguage(pl2b_Program *program,
                          const pl2b_Language *language,
                          void *userContext,
                          pl2b_Error *error) {
  assert(language != NULL);

  RunContext *context = createRunContext(program);
  if (context == NULL) {
    pl2b_errPrintf(error, PL2B_ERR_MALLOC, pl2b_sourceInfo(NULL, 0),
                   NULL, "run: cannot allocate memory for run context");
    return;
  }

  context->language = language;
  context->userContext = userContext;
  if (pl2b_isError(error)) {
    destroyRunContext(context);
    return;
  }

  while (cmdHandler(context, context->curCmd, error)) {
    if (pl2b_isError(error)) {
      break;
    }
  }

  context->userContext = NULL;
  destroyRunContext(context);
}

static RunContext *createRunContext(pl2b_Program *program) {
  RunContext *context = (RunContext*)malloc(sizeof(RunContext));
  if (context == NULL) {
    return NULL;
  }

  context->program = program;
  context->curCmd = program->commands;
  context->userContext = NULL;
  context->language = NULL;
  return context;
}

static void destroyRunContext(RunContext *context) {
  free(context);
}

static _Bool cmdHandler(RunContext *context,
                        pl2b_Cmd *cmd,
                        pl2b_Error *error) {
  if (cmd == NULL) {
    return 0;
  }

  if (!strcmp(cmd->cmd.str, "language")) {
    pl2b_errPrintf(error, PL2B_ERR_NO_LANG, cmd->sourceInfo, NULL,
                   "No need to use language command in PL2BK, "
                   "languages are pre-loaded.");
    return 0;
  } else if (!strcmp(cmd->cmd.str, "abort")) {
    return 0;
  }

  for (pl2b_PCallCmd *iter = context->language->pCallCmds;
       iter != NULL && !PL2B_EMPTY_CMD(iter);
       ++iter) {
    if (!iter->removed && !strcmp(cmd->cmd.str, iter->cmdName)) {
      if (iter->cmdName != NULL
          && strcmp(cmd->cmd.str, iter->cmdName) != 0) {
        /* Do nothing if so */
      } else if (iter->routerStub != NULL
                 && !iter->routerStub(cmd->cmd)) {
        /* Do nothing if so */
      } else {
        if (iter->deprecated) {
          LOG_WARN("using deprecated command: %s\n", iter->cmdName);
        }

        if (iter->stub == NULL) {
          LOG_WARN("entry for command %s exists but NULL\n",
                   cmd->cmd.str);
          context->curCmd = cmd->next;
          return 1;
        }

        pl2b_Cmd *nextCmd = iter->stub(context->program,
                                       context->userContext,
                                       cmd,
                                       error);
        return checkNextCmdRet(context, nextCmd, error);
      }
    }
  }

  if (context->language->fallback == NULL) {
    pl2b_errPrintf(error, PL2B_ERR_UNKNOWN_CMD, cmd->sourceInfo, NULL,
                   "`%s` is not recognized as an internal or external "
                   "command, operable program or batch file",
                   cmd->cmd);
    return 0;
  }

  pl2b_Cmd *nextCmd = context->language->fallback(
    context->program,
    context->userContext,
    cmd,
    error
  );

  return checkNextCmdRet(context, nextCmd, error);
}

static _Bool checkNextCmdRet(RunContext *context,
                             pl2b_Cmd *nextCmd,
                             pl2b_Error *error) {
  if (pl2b_isError(error)) {
    return 0;
  }
  if (nextCmd == NULL) {
    return 0;
  }

  context->curCmd = nextCmd;
  return 1;
}
