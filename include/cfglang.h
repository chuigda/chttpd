/**
 * cfg_lang.h: minimal configuration language for http server
 *
 * Basic grammar of configuration language:
 *   configuration ::= lines
 *   lines ::= lines line | NIL
 *   line ::= router-line | config-line
 *   router-line ::= method PATH handler-type HANDLER
 *   method ::= "get" | "post"
 *   handler-type ::= "script" | "cgi" | "static"
 *   config-line ::= "listen-address" ADDRESS
 *                 | "listen-port" PORT
 */

#ifndef CFG_LANG_H
#define CFG_LANG_H

#include "cc_vec.h"
#include "http_base.h"
#include "pl2b.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum e_handler_type {
  HDLR_SCRIPT = 0,
  HDLR_CGI    = 1,
  HDLR_STATIC = 2
} HandlerType;

extern const char *HANDLER_TYPE_NAMES[];

typedef struct st_route {
  HttpMethod httpMethod;
  const char *path;
  HandlerType handlerType;
  const char *handlerPath;
} Route;

typedef struct st_config {
  const char *address;
  int port;

  ccVec TP(Route) routes;
} Config;

void initConfig(Config *config);
void dropConfig(Config *config);

const pl2b_Language *getCfgLanguage(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CFG_LANG_H */
