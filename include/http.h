#ifndef HTTP_H
#define HTTP_H

#include "cc_vec.h"
#include "http_base.h"

typedef enum e_http_error {
  HTTP_ERR_NO_ERROR = 0,

  HTTP_ERR_PROTOCOL = 1,
  HTTP_ERR_IO       = 2,
  HTTP_ERR_INTERNAL = 3
} HttpError;

typedef struct st_http_request {
  HttpMethod method;
  size_t contentLength;
  ccVec params;
  ccVec headers;
  char body[0];
} HttpRequest;

HttpRequest *readHttpRequest(int fd);
void dropHttpRequest(HttpRequest *request);

typedef struct st_http_response {
  HttpCode code;
  const char *statusText;
  size_t contentLength;
  ccVec headers;
  char body[0];
} HttpResponse;

HttpResponse *createHttpResponse(HttpCode code,
                                 const char *statusText,
                                 ccVec headers,
                                 const char *body);
int writeHttpResponse(int fd, HttpResponse *response);
void dropHttpResponse(HttpResponse *response);

#endif /* HTTP_H */