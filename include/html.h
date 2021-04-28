/**
 * html.h: defines a representation for HTML document
 */

#ifndef HTML_H
#define HTML_H

#include "cc_vec.h"
#include "util.h"

#include <stdio.h>

typedef enum e_http_doc_tag_type {
  HTML_HTML       = 1,   /* <html>            */
  HTML_TITLE      = 2,   /* <title>           */
  HTML_HEAD       = 3,   /* <head>            */
  HTML_BODY       = 4,   /* <body>            */
  HTML_META       = 5,   /* <meta>            */
  HTML_BR         = 6,   /* <br/>             */
  HTML_HR         = 7,   /* <hr/>             */
  HTML_H1         = 8,   /* <h1>              */
  HTML_H2         = 9,   /* <h2>              */
  HTML_H3         = 10,  /* <h3>              */
  HTML_H4         = 11,  /* <h4>              */
  HTML_H5         = 12,  /* <h5>              */
  HTML_H6         = 13,  /* <h6>              */
  HTML_DIV        = 14,  /* <div>             */
  HTML_PARA       = 15,  /* <p>               */
  HTML_SPAN       = 16,  /* <span>            */
  HTML_BOLD       = 17,  /* <b>               */
  HTML_DELETE     = 18,  /* <del>             */
  HTML_FORM       = 19,  /* <form>            */
  HTML_INPUT      = 20,  /* <input>           */
  HTML_TEXT       = 21,  /* <textarea>        */

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
HtmlDoc *htmlScriptUrlNode(const char *url);
HtmlDoc *htmlScriptNode(const char *scriptText);

HtmlTagType htmlGetTag(const HtmlDoc *doc);
_Bool htmlCanHaveAttr(const HtmlDoc *doc);
_Bool htmlCanHaveSubDoc(const HtmlDoc *doc);
void htmlAddAttr(HtmlDoc *doc, StringPair attrPair);
void htmlAddSubDoc(HtmlDoc *doc, HtmlDoc *subDoc);
void deleteHtml(HtmlDoc *doc);

int printHtml(FILE *fp, HtmlDoc *doc);

#endif /* HTML_H */
