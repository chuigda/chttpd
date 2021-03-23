#ifndef HTML_H
#define HTML_H

#include "cc_vec.h"
#include "util.h"

typedef enum e_http_doc_tag_type {
  HTML_HTML       = 1,   /* <html>            */
  HTML_TITLE      = 2,   /* <title>           */
  HTML_BR         = 3,   /* <br/>             */
  HTML_HR         = 4,   /* <hr/>             */
  HTML_H1         = 5,   /* <h1>              */
  HTML_H2         = 6,   /* <h2>              */
  HTML_H3         = 7,   /* <h3>              */
  HTML_H4         = 8,   /* <h4>              */
  HTML_H5         = 9,   /* <h5>              */
  HTML_H6         = 10,  /* <h6>              */
  HTML_DIV        = 11,  /* <div>             */
  HTML_PARA       = 12,  /* <p>               */
  HTML_SPAN       = 13,  /* <span>            */
  HTML_BOLD       = 14,  /* <b>               */
  HTML_DELETE     = 15,  /* <del>             */
  HTML_FORM       = 16,  /* <form>            */
  HTML_INPUT      = 17,  /* <input>           */
  HTML_TEXT       = 18,  /* <textarea>        */

  HTML_SCRIPT_URL = 198, /* <script url="">   */
  HTML_SCRIPT     = 199, /* <script></script> */

  HTML_CUSTOM     = 200, /* <user-defined>    */
  HTML_PLAIN      = 201, /* plain text node   */
} HtmlTagType;

const char *htmlTagRepr(HtmlTagType tagType);

typedef struct st_html_doc {
  max_align_t dummy;
} HtmlDoc;

HtmlDoc *htmlRootDoc(void);
HtmlDoc *htmlTag(HtmlTagType tag);
HtmlDoc *htmlCustomTag(const char *customTag);
HtmlDoc *htmlPlainNode(const char *plainText);

HtmlTagType htmlGetTag(const HtmlDoc *doc);
_Bool htmlCanHaveAttr(const HtmlDoc *doc);
_Bool htmlCanHaveSubDoc(const HtmlDoc *doc);
void addAttribute(HtmlDoc *doc, StringPair attrPair);
void addSubDoc(HtmlDoc *doc, HtmlDoc *subDoc);

void deleteHtml(HtmlDoc *doc);

char* htmlDocToString(HtmlDoc *htmlDoc);

#endif /* HTML_H */
