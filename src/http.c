#include "http.h"
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const HttpMethod HTTP_ALL_METHODS[] = {
  HTTP_GET,
  HTTP_POST,
  HTTP_OPTIONS
};

const char *HTTP_METHOD_NAMES[] = {
  [HTTP_GET] = "GET",
  [HTTP_POST] = "POST",
  [HTTP_OPTIONS] = "OPTIONS"
};

const char *HTTP_CODE_NAMES[] = {
  [HTTP_CODE_OK] = "Ok",
  [HTTP_CODE_NO_CONTENT] = "No Content",
  [HTTP_CODE_BAD_REQUEST] = "Bad Request",
  [HTTP_CODE_UNAUTHORIZED] = "Unauthorised",
  [HTTP_CODE_FORBIDDEN] = "Forbidden",
  [HTTP_CODE_NOT_FOUND] = "Not Found",
  [HTTP_CODE_SERVER_ERR] = "Internal Server Error",
};

const char *HTTP_CORS_HEADERS = 
"Access-Control-Allow-Origin: *\r\n"
"Access-Control-Allow-Headers: *\r\n";

HttpMethod parseHttpMethod(const char *methodStr, _Bool *error) {
  const char *const *METHOD_NAMES = HTTP_METHOD_NAMES;
  if (strcmp_icase(methodStr, METHOD_NAMES[HTTP_GET])) {
    return HTTP_GET;
  } else if (strcmp_icase(methodStr, METHOD_NAMES[HTTP_POST])) {
    return HTTP_POST;
  } else if (strcmp_icase(methodStr, METHOD_NAMES[HTTP_OPTIONS])) {
    return HTTP_OPTIONS;
  } else {
    if (error != NULL) {
      *error = 1;
    }
    return HTTP_GET;
  }
}

HttpMethod parseHttpMethodSlice(const char *begin,
                                const char *end,
                                _Bool *error) {
  const char *const *METHOD_NAMES = HTTP_METHOD_NAMES;
  if (slicecmp_icase(begin, end, METHOD_NAMES[HTTP_GET])) {
    return HTTP_GET;
  } else if (slicecmp_icase(begin, end, METHOD_NAMES[HTTP_POST])) {
    return HTTP_POST;
  } else if (slicecmp_icase(begin, end, METHOD_NAMES[HTTP_OPTIONS])) {
    return HTTP_OPTIONS;
  } else {
    if (error != NULL) {
      *error = 1;
    }
    return HTTP_GET;
  }
}

const char *httpCodeNameSafe(int httpCode) {
  const char *ret = HTTP_CODE_NAMES[httpCode];
  if (ret == NULL) {
    ret = "Unknown";
  }
  return ret;
}

void dropHttpRequest(HttpRequest *request) {
  for (size_t i = 0; i < ccVecLen(&request->headers); i++) {
    StringPair *header = (StringPair*)ccVecNth(&request->headers, i);
    dropStringPair(*header);
  }
  ccVecDestroy(&request->headers);

  for (size_t i = 0; i < ccVecLen(&request->params); i++) {
    StringPair *param = (StringPair*)ccVecNth(&request->params, i);
    dropStringPair(*param);
  }
  ccVecDestroy(&request->params);

  free(request->requestPath);
  free(request->queryString);
  free(request);
}

static char *readHttpLine(FILE *fp);
static _Bool parseHttpFirstLine(const char *line,
                                HttpMethod *method,
                                char **requestPath,
                                char **queryString,
                                ccVec TP(StringPair) *params);
static _Bool parseHttpHeaderLine(const char *line,
                                 ccVec TP(StringPair) *headers);
static const char *skipWhitespace(const char *start);
static _Bool parseQueryPath(const char *start,
                            const char *end,
                            char **requestPath,
                            char **queryString,
                            ccVec TP(StringPair) *params);

HttpRequest *readHttpRequest(FILE *fp) {
  char *line = readHttpLine(fp);
  if (line == NULL) {
    return NULL;
  }

  HttpMethod method;
  ccVec TP(StringPair) params;
  ccVecInit(&params, sizeof(StringPair));
  char *requestPath = NULL;
  char *queryString = NULL;

  if (!parseHttpFirstLine(
        line,
        &method,
        &requestPath,
        &queryString,
        &params
     )) {
    goto free_params_ret;
  }
  free(line);

  ccVec TP(StringPair) headers;
  ccVecInit(&headers, sizeof(StringPair));

  for (;;) {
    line = readHttpLine(fp);
    if (line == NULL) {
      goto free_headers_ret;
    }

    if (strlen(line) == 2) {
      free(line);
      break;
    }

    if (!parseHttpHeaderLine(line, &headers)) {
      goto free_headers_ret;
    }
    free(line);
  }

  size_t contentLength = 0;
  for (size_t i = 0; i < ccVecLen(&headers); i++) {
    const StringPair *header = (const StringPair*)ccVecNth(&headers, i);
    if (strcmp_icase(header->first, "Content-Length")) {
      int length = atoi(header->second);
      if (length <= 0) {
        LOG_WARN("invalid Content-Length header value \"%s\" ignored.",
                 header->second);
      } else {
        contentLength = length;
      }
      break;
    }
  }

  HttpRequest *ret = 
    (HttpRequest*)malloc(sizeof(HttpRequest) + contentLength + 1);
  if (ret == NULL) {
    goto free_headers_ret;
  }

  if (contentLength != 0) {
    ssize_t bytesRead = fread(ret->body, 1, contentLength, fp);
    if (bytesRead < (ssize_t)contentLength) {
      LOG_ERR("failed to read body of %zu length\n",
              contentLength);
      goto free_all_ret;
    }
  }
  ret->body[contentLength] = '\0';

  ret->method = method;
  ret->contentLength = contentLength;
  ret->requestPath = requestPath;
  ret->queryString = queryString;
  ret->params = params;
  ret->headers = headers;
  return ret;

free_all_ret:
  free(ret);
  /* fallthrough */

free_headers_ret:
  for (size_t i = 0; i < ccVecLen(&headers); i++) {
    StringPair *header = (StringPair*)ccVecNth(&headers, i);
    dropStringPair(*header);
  }
  ccVecDestroy(&headers);
  /* fallthrough */

free_params_ret:
  free(requestPath);
  free(queryString);
  for (size_t i = 0; i < ccVecLen(&params); i++) {
    StringPair *param = (StringPair*)ccVecNth(&params, i);
    dropStringPair(*param);
  }
  ccVecDestroy(&params);
  free(line);
  return NULL;
}

static char *readHttpLine(FILE *fp) {
  char *line = NULL;
  size_t n = 0;
  ssize_t lineSize = getdelim(&line, &n, '\n', fp);

  if (lineSize == -1) {
    if (errno != 0) {
      LOG_ERR("error: getdelim: %d", errno);
    }
    free(line);
    return NULL;
  }

  if (lineSize > 2
      && (line[lineSize - 1] != '\n'
          || line[lineSize - 2] != '\r')) {
    LOG_ERR("error: http line not ending with \"\\r\\n\"");
    free(line);
    return NULL;
  }

  return line;
}

static _Bool parseHttpFirstLine(const char *line,
                                HttpMethod *method,
                                char **requestPath,
                                char **queryString,
                                ccVec TP(StringPair) *params) {
  const char *it = strchr(line, ' ');
  if (it == NULL) {
    LOG_ERR("error parsing http request: \"%s\": missing first space",
            line);
    return 0;
  }

  _Bool parseError = 0;
  *method = parseHttpMethodSlice(line, it, &parseError);
  if (parseError) {
    LOG_ERR("error parsing http request: \"%s\": unsupported method",
            line);
    return 0;
  }

  it = skipWhitespace(it);
  const char *it2 = strchr(it, '/');
  if (it2 == NULL) {
    LOG_ERR("error parsing http request: \"%s\": missing path",
            line);
    return 0;
  }
  it = it2;
  it2 = strchr(it2, ' ');

  if (!parseQueryPath(it, it2, requestPath, queryString, params)) {
    LOG_ERR("error parsing http request: \"%s\": invalid query path",
            line);
    return 0;
  }

  it2 = skipWhitespace(it2);
  if (strncmp(it2, "HTTP/1.1", 8)) {
    LOG_ERR("error parsing http request: \"%s\": invalid http request");
    return 0;
  }

  return 1;
}

static _Bool parseHttpHeaderLine(const char *line,
                                 ccVec TP(StringPair) *headers) {
  const char *it = strchr(line, ':');
  if (it == NULL) {
    LOG_ERR("error parsing http header: \"%s\": missing colon", line);
    return 0;
  }

  size_t nameSize = it - line;
  char *name = (char*)malloc(nameSize + 1);
  strncpy(name, line, nameSize);
  name[nameSize] = '\0';

  it++;
  it = skipWhitespace(it);
  const char *it2 = it;
  while (*it2 != '\r') it2++;

  size_t valueSize = it2 - it;
  char *value = (char*)malloc(valueSize + 1);
  strncpy(value, it, valueSize);
  value[valueSize] = '\0';

  StringPair header = (StringPair) { name, value };
  ccVecPushBack(headers, &header);

  return 1;
}

static const char *skipWhitespace(const char *str) {
  while (isspace(*str) && *str != '\r' && *str != '\n') {
    ++str;
  }
  return str;
}

static _Bool parseQueryPath(const char *it1,
                            const char *it2,
                            char **requestPath,
                            char **queryString,
                            ccVec TP(StringPair) *params) {
  const char *it3 = it1;
  while (it3 != it2 && *it3 != '?') {
    it3++;
  }
  
  size_t pathSize = it3 - it1;
  *requestPath = (char*)malloc(pathSize + 1);
  strncpy(*requestPath, it1, pathSize);
  (*requestPath)[pathSize] = '\0';

  if (*it3 != '?') {
    return 1;
  }

  it3++;

  size_t queryStringSize = it2 - it3;
  *queryString = (char*)malloc(queryStringSize + 1);
  strncpy(*queryString, it3, queryStringSize);
  (*queryString)[queryStringSize] = '\0';

  for (;;) {
    it1 = it3;
    while (it3 != it2 && *it3 != '=') {
      it3++;
    }

    if (*it3 != '=') {
      LOG_ERR("error parsing query parameter: \"=\" expected");
      return 0;
    }
    const char *it4 = it3 + 1;
    while (it4 != it2 && *it4 != '&') {
      it4++;
    }
 
    size_t keySize = it3 - it1;
    size_t valueSize = it4 - it3 - 1;
    char *key = (char*)malloc(keySize + 1);
    char *value = (char*)malloc(valueSize + 1);
    strncpy(key, it1, keySize);
    key[keySize] = '\0';
    strncpy(value, it3 + 1, valueSize);
    value[valueSize] = '\0';
    
    StringPair param = (StringPair) { key, value };
    ccVecPushBack(params, &param);

    if (*it4 != '&') {
      return 1;
    }
  
    it3 = it4 + 1;
  }
}

