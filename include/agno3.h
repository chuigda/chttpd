/**
 * agno3.h: The PL2-based HTML document generator
 *
 * http://www.yinwang.org/blog-cn/2017/05/25/dsl
 * AgNO3 + NaCl = AgCl↓ + NaNO3
 */

#ifndef AGNO3_H
#define AGNO3_H

#include "cc_vec.h"
#include "http.h"
#include "html.h"
#include "pl2b.h"

typedef void *Flask;

typedef struct st_http_response_head {
  int code;
  const char *statusText;
  ccVec TP(StringPair) headers;
} HttpResponseHead;

Flask createFlask(HttpRequest request);
HttpResponseHead extractResponseHead(Flask flask);
HtmlDoc *extractHtmlDoc(Flask flask);
void drainFlask(Flask flask);

const pl2b_Language *getAgNO3(void);

#endif /* AGNO3_H */
