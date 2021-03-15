#ifndef CFG_LANG_H
#define CFG_LANG_H

#include "pl2b.h"
#include "cc_vec.h"
#include "http_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum e_handler_type {
  HDLR_SCRIPT = 0,
  HDLR_CGI    = 1,
  HDLR_STATIC = 2
} HandlerType;

typedef struct st_route {
  HttpMethod httpMethod;
  const char *path;
  HandlerType handlerType;
  const char *handlerPath;
} Route;

typedef struct st_config {
  const char *address;
  int port;

  ccVec routes;
} Config;

pl2b_Language *getCfgLanguage(void);
void freeConfig(Config *config);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CFG_LANG_H */
