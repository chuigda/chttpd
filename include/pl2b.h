#ifndef PLAPI_PL2B_H
#define PLAPI_PL2B_H

#define PL2B_API

/*** -------------------------- versioning ------------------------- ***/

#define PL2_EDITION       "PL2-BK"       /* Latin name */
#define PL2_EDITION_CN    "霹雳2-丙-K"   /* Chinese name */
#define PL2_EDITION_RU    "ПЛ2-БК"       /* Cyrillic name */
#define PL2_EDITION_JP    "ピリ-2BK"     /* Japanese name */
#define PL2_EDITION_KR    "펠리-2BK"     /* Kroean name */
#define PL2B_VER_MAJOR    0              /* Major version of PL2BK */
#define PL2B_VER_MINOR    3              /* Minor version of PL2BK */
#define PL2B_VER_PATCH    0              /* Patch version of PL2BK */
#define PL2B_VER_POSTFIX  "BlueGiant"    /* Version postfix */

const char *pl2b_getLocaleName(void);

/*** ---------------------- end configurations --------------------- ***/

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*** ------------------------ pl2b_CmpResult ----------------------- ***/

typedef enum e_pl2b_cmp_result {
  PL2B_CMP_LESS    = -1, /* LHS < RHS */
  PL2B_CMP_EQ      = 0,  /* LHS = RHS */
  PL2B_CMP_GREATER = 1,  /* LHS > RHS */
  PL2B_CMP_NONE    = 255 /* not comparable */
} pl2b_CmpResult;

/*** ----------------------- pl2b_SourceInfo  ---------------------- ***/

typedef struct st_pl2b_source_info {
  const char *fileName;
  uint16_t line;
} pl2b_SourceInfo;

pl2b_SourceInfo pl2b_sourceInfo(const char *fileName, uint16_t line);

/*** -------------------------- pl2b_Error ------------------------- ***/

typedef struct st_pl2b_error {
  void *extraData;
  pl2b_SourceInfo sourceInfo;
  uint16_t errorCode;
  uint16_t errorBufferSize;
  char reason[0];
} pl2b_Error;

typedef enum e_pl2b_error_code {
  PL2B_ERR_NONE           = 0,  /* no error */
  PL2B_ERR_GENERAL        = 1,  /* general hard error */
  PL2B_ERR_PARSEBUF       = 2,  /* parse buffer exceeded */
  PL2B_ERR_UNCLOSED_STR   = 3,  /* unclosed string literal */
  PL2B_ERR_UNCLOSED_BEGIN = 4,  /* unclosed ?begin block */
  PL2B_ERR_EMPTY_CMD      = 5,  /* empty command */
  PL2B_ERR_SEMVER_PARSE   = 6,  /* semver parse error */
  PL2B_ERR_UNKNOWN_QUES   = 7,  /* unknown question mark command */
  PL2B_ERR_LOAD_LANG      = 8,  /* error loading language */
  PL2B_ERR_NO_LANG        = 9,  /* language not loaded */
  PL2B_ERR_UNKNOWN_CMD    = 10, /* unknown command */
  PL2B_ERR_MALLOC         = 11, /* malloc failure*/

  PL2B_ERR_USER           = 100 /* generic user error */
} pl2b_ErrorCode;

pl2b_Error *pl2b_errorBuffer(uint16_t strBufferSize);

void pl2b_errPrintf(pl2b_Error *error,
                    uint16_t errorCode,
                    pl2b_SourceInfo sourceInfo,
                    void *extraData,
                    const char *fmt,
                    ...);

void pl2b_dropError(pl2b_Error *error);

_Bool pl2b_isError(pl2b_Error *error);

/*** --------------------------- pl2b_Cmd -------------------------- ***/

typedef struct st_pl2b_cmd_part {
  char *str;
  _Bool isString;
} pl2b_CmdPart;

pl2b_CmdPart pl2b_cmdPart(char *str, _Bool isString);

#define PL2B_EMPTY_PART(cmdPart) (!((cmdPart).str))

typedef struct st_pl2b_cmd {
  struct st_pl2b_cmd *prev;
  struct st_pl2b_cmd *next;

  pl2b_SourceInfo sourceInfo;
  pl2b_CmdPart cmd;
  pl2b_CmdPart args[0];
} pl2b_Cmd;

pl2b_Cmd *pl2b_cmd3(pl2b_SourceInfo sourceInfo,
                    pl2b_CmdPart cmd,
                    pl2b_CmdPart args[]);

pl2b_Cmd *pl2b_cmd5(pl2b_Cmd *prev,
                    pl2b_Cmd *next,
                    pl2b_SourceInfo sourceInfo,
                    pl2b_CmdPart cmd,
                    pl2b_CmdPart args[]);

uint16_t pl2b_argsLen(pl2b_Cmd *cmd);

/*** ------------------------- pl2b_Program ------------------------ ***/

typedef struct st_pl2b_program {
  pl2b_Cmd *commands;
} pl2b_Program;

void pl2b_initProgram(pl2b_Program *program);
pl2b_Program pl2b_parse(char *source,
                        uint16_t parseBufferSize,
                        pl2b_Error *error);
void pl2b_dropProgram(pl2b_Program *program);

/*** -------------------- Semantic-ver parsing  -------------------- ***/

#define PL2B_SEMVER_POSTFIX_LEN 15

typedef struct st_pl2b_sem_ver {
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
  char postfix[PL2B_SEMVER_POSTFIX_LEN];
  _Bool exact;
} pl2b_SemVer;

pl2b_SemVer pl2b_zeroVersion(void);
pl2b_SemVer pl2b_parseSemVer(const char *src, pl2b_Error *error);
_Bool pl2b_isZeroVersion(pl2b_SemVer ver);
_Bool pl2b_isAlpha(pl2b_SemVer ver);
_Bool pl2b_isStable(pl2b_SemVer ver);
_Bool pl2b_isCompatible(pl2b_SemVer expected, pl2b_SemVer actual);
pl2b_CmpResult pl2b_semverCmp(pl2b_SemVer ver1, pl2b_SemVer ver2);
void pl2b_semverToString(pl2b_SemVer ver, char *buffer);

/*** ------------------------ pl2b_Extension ----------------------- ***/

typedef pl2b_Cmd *(pl2b_PCallCmdStub)(pl2b_Program *program,
                                      void *context,
                                      pl2b_Cmd *command,
                                      pl2b_Error *error);
typedef _Bool (pl2b_CmdRouterStub)(pl2b_CmdPart cmd);

typedef void *(pl2b_InitStub)(pl2b_Error *error);
typedef void (pl2b_AtexitStub)(void *context);
typedef void (pl2b_CmdCleanupStub)(void *cmdExtra);

typedef struct st_pl2b_pcall_func {
  const char *cmdName;
  pl2b_CmdRouterStub *routerStub;
  pl2b_PCallCmdStub *stub;
  _Bool deprecated;
  _Bool removed;
} pl2b_PCallCmd;

#define PL2B_EMPTY_SINVOKE_CMD(cmd) \
  ((cmd)->cmdName == 0 && (cmd)->stub == 0)
#define PL2B_EMPTY_CMD(cmd) \
  ((cmd)->cmdName == 0 && (cmd)->routerStub == 0 && (cmd)->stub == 0)

typedef struct st_pl2b_langauge {
  const char *langName;
  const char *langInfo;

  pl2b_PCallCmd *pCallCmds;
  pl2b_PCallCmdStub *fallback;
} pl2b_Language;

typedef pl2b_Language *(pl2b_LoadLanguage)(pl2b_SemVer version,
                                           pl2b_Error *error);

/*** ----------------------------- Run ----------------------------- ***/

void* pl2b_runWithLanguage(pl2b_Program *program,
                           pl2b_Language *language,
                           void *userContext,
                           pl2b_Error *error);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PLAPI_PL2B_H */
