#ifndef HTML_H
#define HTML_H

#include "cc_vec.h"
#include "util.h"

typedef enum e_http_doc_tag_type {
  HTML_HTML   = 1,  /* <html>     */
  HTML_TITLE  = 2,  /* <title>    */
  HTML_SCRIPT = 3,  /* <script>   */
  HTML_BR     = 4,  /* <br/>      */
  HTML_HR     = 5,  /* <hr/>      */
  HTML_H1     = 6,  /* <h1>       */
  HTML_H2     = 7,  /* <h2>       */
  HTML_H3     = 8,  /* <h3>       */
  HTML_H4     = 9,  /* <h4>       */
  HTML_H5     = 10, /* <h5>       */
  HTML_H6     = 11, /* <h6>       */
  HTML_DIV    = 12, /* <div>      */
  HTML_PARA   = 13, /* <p>        */
  HTML_SPAN   = 14, /* <span>     */
  HTML_BOLD   = 15, /* <b>        */
  HTML_DELETE = 16, /* <del>      */
  HTML_FORM   = 17, /* <form>     */
  HTML_INPUT  = 18, /* <input>    */
  HTML_TEXT   = 19, /* <textarea> */

  HTML_CUSTOM = 100, /* <?????> */

  HTML_PLAIN  = 101, /* plain text node  */
} HtmlTagType;

const char *htmlTagRepr(HtmlTagType tagType);

typedef struct st_html_doc {
  // TODO make this structure opaque
  HtmlTagType tagType;
  Owned(char*) customTagName;

  ccVec TP(Owned(StringPair)) attrs;
  union {
    ccVec TP(HtmlDoc) subDocs;
    Owned(char*) plain;
  } data;
} HtmlDoc;

Owned(char*) htmlDocToString(HtmlDoc *htmlDoc);

HtmlDoc *htmlRootDoc(void);
HtmlDoc *htmlTag(HtmlTagType tag);
HtmlDoc *htmlCustomTag(const char *customTag);
HtmlDoc *htmlPlainNode(const char *plainText);

HtmlTagType getHtmlTagType(const HtmlDoc *doc);
void addAttribute(HtmlDoc *doc, Ref(StringPair) attrPair);

void deleteHtml(HtmlDoc *doc);

#endif /* HTML_H */
