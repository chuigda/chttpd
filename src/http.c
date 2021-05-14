#include "http.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char *HTTP_METHOD_NAMES[] = {
  [HTTP_GET] = "GET",
  [HTTP_POST] = "POST"
};

const char *HTTP_CODE_NAMES[] = {
  [HTTP_CODE_OK] = "Ok",
  [HTTP_CODE_NO_CONTENT] = "No Content",
  [HTTP_CODE_BAD_REQUEST] = "Bad Request",
  [HTTP_CODE_UNAUTHORIZED] = "Unauthorised",
  [HTTP_CODE_FORBIDDEN] = "Forbidden",
  [HTTP_CODE_NOT_FOUND] = "Not Found",
  [HTTP_CODE_SERVER_ERR] = "Internal Server Error"
};

static _Bool parseHttpFirstLine(const char *line,
                                HttpMethod *method,
                                ccVec TP(StringPair) params,
                                ccVec TP(StringPair) headers);

HttpRequest *readHttpRequest(FILE *fp) {
  char *firstLine = NULL;
  ssize_t lineSize = getdelim(&firstLine, 0, '\n', fp);

  if (lineSize == -1) {
    if (errno != 0) {
      LOG_ERR("error: getdelim: %d", errno);
      free(firstLine);
      return NULL;
    }
  }

  HttpMethod method;
  ccVec TP(StringPair) params;
  ccVec TP(StringPair) headers;

  ccVecInit(&params, sizeof(StringPair));
  ccVecInit(&headers, sizeof(StringPair));

  // TODO
  return NULL;
}

