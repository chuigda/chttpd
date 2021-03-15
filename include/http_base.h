#ifndef HTTP_BASE_H
#define HTTP_BASE_H

typedef enum e_http_method {
  HTTP_GET  = 0,
  HTTP_POST = 1
} HttpMethod;

typedef enum e_http_code {
  HTTP_CODE_OK            = 200,
  HTTP_CODE_NO_CONTENT    = 204,
  HTTP_CODE_BAD_REQUEST   = 400,
  HTTP_CODE_UNAUTHORIZED  = 401,
  HTTP_CODE_FORBIDDEN     = 403,
  HTTP_CODE_NOT_FOUND     = 404,
  HTTP_CODE_SERVER_ERR    = 500
} HttpCode;

#endif /* HTTP_BASE_H */
