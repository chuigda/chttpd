/**
 * Basic grammar of configuration language:
 *   configuration ::= lines
 *   lines ::= lines line | NIL
 *   line ::= router-line | filter-line | config-line | cors-line
 *   cors-line ::= "cors" method PATH
 *   router-line ::= method PATH handler-type HANDLER
 *   method ::= "get" | "post"
 *   handler-type ::= "dcgi" | "static" | "intern"
 *   config-line ::= "listen-address" ADDRESS
 *                 | "listen-port" PORT
 *                 | "max-pending" MAX-PENDING
 *                 | "preload"     PRELOAD
 *                 | "cache-time"  CACHE-TIME
 *                 | "ignore-case" IGNORE-CASE
 */

#ifndef CHTTPD_CONFIG_H
#define CHTTPD_CONFIG_H

#include "cc_vec.h"
#include "http_base.h"
#include "pl2b.h"
#include "util.h"

#define CHTTPD_VER_MAJOR 0
#define CHTTPD_VER_MINOR 2
#define CHTTPD_VER_PATCH 0

#define CHTTPD_NAME        "chttpd"
#define CHTTPD_SERVER_NAME "chttpd/bravo1"

typedef enum e_handler_type {
  HDLR_STATIC = 1,
  HDLR_DCGI   = 2,
  HDLR_INTERN = 3,
  HDLR_DIR    = 4
} HandlerType;

extern const char *HANDLER_TYPE_NAMES[];

typedef struct st_route {
  HttpMethod httpMethod;
  const char *path;
  HandlerType handlerType;
  const char *handlerPath;

  void *extra;
} Route;

typedef struct st_cors_config {
  HttpMethod httpMethod;
  const char *path;
} CorsConfig;

typedef struct st_config {
  const char *address;
  int port;
  int maxPending;
  _Bool preloadDynamic;
  _Bool ignoreCase;
  int cacheTime;

  ccVec TP(Route) routes;
  ccVec TP(CorsConfig) corsConfig;
} Config;

void initConfig(Config *config);
void dropConfig(Config *config);

const pl2b_Language *getCfgLanguage(void);

#endif /* CHTTPD_CONFIG_H */
