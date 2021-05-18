#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>

#include "cc_vec.h"
#include "http_base.h"
#include "util.h"

typedef enum e_http_error {
  HTTP_ERR_NO_ERROR = 0,

  HTTP_ERR_PROTOCOL = 1,
  HTTP_ERR_IO       = 2,
  HTTP_ERR_INTERNAL = 3
} HttpError;

typedef struct st_http_request {
  HttpMethod method;
  size_t contentLength;
  char *requestPath;
  char *queryString;
  ccVec TP(StringPair) params;
  ccVec TP(StringPair) headers;
  char body[0];
} HttpRequest;

HttpRequest *readHttpRequest(FILE *fp);
void dropHttpRequest(HttpRequest *request);

typedef struct st_http_response {
  HttpCode code;
  const char *statusText;
  size_t contentLength;
  ccVec TP(StringPair) headers;
  char body[0];
} HttpResponse;

HttpResponse *createHttpResponse(HttpCode code,
                                 const char *statusText,
                                 ccVec headers,
                                 const char *body);
int writeHttpResponse(FILE *fp, HttpResponse *response);
void dropHttpResponse(HttpResponse *response);

#endif /* HTTP_H */
