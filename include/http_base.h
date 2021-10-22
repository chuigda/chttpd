#ifndef CHTTPD_HTTP_BASE_H
#define CHTTPD_HTTP_BASE_H

typedef enum e_http_method {
  HTTP_GET     = 0x01,
  HTTP_POST    = 0x02,
  HTTP_OPTIONS = 0x04,
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

extern const HttpMethod HTTP_ALL_METHODS[];
extern const char *HTTP_METHOD_NAMES[];
extern const char *HTTP_CODE_NAMES[];

const char *httpCodeNameSafe(int httpCode);

HttpMethod parseHttpMethod(const char *methodStr, _Bool *error);
HttpMethod parseHttpMethodSlice(const char *begin,
                                const char *end,
                                _Bool *error);

extern const char *HTTP_CORS_HEADERS;

#endif /* HTTP_BASE_H */
