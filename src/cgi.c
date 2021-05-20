#include "cgi.h"

#include <ctype.h>  
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cc_vec.h"

static ccVec TP(char*) requestToEnv(const Config *config,
                                    const HttpRequest *request,
                                    const char *clientAddr,
                                    const char *scriptName);
static char *buildEnvItem(const char *key, const char *value);

void handleCGI(const Config *config,
               const char *cgiPath,
               const HttpRequest *request,
               const char *clientAddr,
               FILE *fp,
               Error *error) {
  ccVec TP(char*) env = requestToEnv(config,
                                     request,
                                     clientAddr,
                                     cgiPath);
  int childStdin[2];
  int childStdout[2];

  if (pipe(childStdin) < 0) {
    QUICK_ERROR2(error, 500, "cannot create pipe for child stdin: %d",
                 errno);
    return;
  }
  if (pipe(childStdout) < 0) {
    QUICK_ERROR2(error, 500, "cannot create pipe for child stdout: %d",
                 errno);
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    QUICK_ERROR2(error, 500, "cannot create CGI process: %d", errno);
    return;
  }

  if (pid == 0) {
    close(childStdin[1]);
    close(childStdout[0]);
    dup2(childStdin[0], STDIN_FILENO);
    dup2(childStdout[1], STDOUT_FILENO);
    alarm(config->cgiTimeout);
    char *const argv[] = { copyString(cgiPath), NULL };
    execvpe(cgiPath, argv, (char**)ccVecData(&env));
  } else {
    close(childStdin[0]);
    close(childStdout[1]);
    
    if (request->contentLength != 0) {
      write(childStdin[0], request->body, request->contentLength);
    }

    FILE *childOutput = fdopen(childStdout[0], "r");
    /* TODO parse output */
  }
}

static ccVec TP(char*) requestToEnv(const Config *config,
                                    const HttpRequest *request,
                                    const char *clientAddr,
                                    const char *scriptName) {
  char *requestMethod = buildEnvItem("REQUEST_METHOD",
                                     HTTP_METHOD_NAMES[request->method]);
  char *queryString = buildEnvItem("QUERY_STRING", request->queryString);
  char *serverName = buildEnvItem("SERVER_NAME", config->address);
  char *serverSoft = buildEnvItem("SERVER_SOFTWARE", CHTTPD_SERVER_NAME);
  char *scName = buildEnvItem("SCRIPT_NAME", scriptName);
  char *gatewayInterface = buildEnvItem("GATEWAY_INTERFACE", "CGI/1.1");
  char *remoteAddr = buildEnvItem("REMOTE_ADDR", clientAddr);
  /* TODO setup server port */

  ccVec TP(char*) ret;
  ccVecInit(&ret, sizeof(char*));
  ccVecPushBack(&ret, &requestMethod);
  ccVecPushBack(&ret, &queryString);
  ccVecPushBack(&ret, &serverName);
  ccVecPushBack(&ret, &serverSoft);
  ccVecPushBack(&ret, &scName);
  ccVecPushBack(&ret, &gatewayInterface);
  ccVecPushBack(&ret, &remoteAddr);

  for (size_t i = 0; i < ccVecLen(&request->headers); i++) {
    StringPair *header = (StringPair*)ccVecNth(&request->headers, i);
    char *envItem = buildEnvItem(header->first, header->second);
    ccVecPushBack(&ret, &envItem);
  }

  return ret;
}

static char *buildEnvItem(const char *key, const char *value) {
  char *ret = (char*)malloc(strlen(key) + strlen(value) + 1);
  char *iter = ret;
  while (*key != '\0') {
    if (*key == '-') {
      *iter = '_';
    } else {
      *iter = toupper(*key);
    }
    iter++;
    key++;
  }
  *iter++ = '=';
  strcpy(iter, value);
  return ret;
}
