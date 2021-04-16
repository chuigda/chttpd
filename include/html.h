#ifndef HTML_H
#define HTML_H

#include "cc_vec.h"
#include "util.h"

#include <stdio.h>

typedef enum e_http_doc_tag_type {
  HTML_HTML       = 1,   /* <html>            */
  HTML_TITLE      = 2,   /* <title>           */
  HTML_BODY       = 3,   /* <body>            */
  HTML_META       = 4,   /* <meta>            */
  HTML_BR         = 5,   /* <br/>             */
  HTML_HR         = 6,   /* <hr/>             */
  HTML_H1         = 7,   /* <h1>              */
  HTML_H2         = 8,   /* <h2>              */
  HTML_H3         = 9,   /* <h3>              */
  HTML_H4         = 10,  /* <h4>              */
  HTML_H5         = 11,  /* <h5>              */
  HTML_H6         = 12,  /* <h6>              */
  HTML_DIV        = 13,  /* <div>             */
  HTML_PARA       = 14,  /* <p>               */
  HTML_SPAN       = 15,  /* <span>            */
  HTML_BOLD       = 16,  /* <b>               */
  HTML_DELETE     = 17,  /* <del>             */
  HTML_FORM       = 18,  /* <form>            */
  HTML_INPUT      = 19,  /* <input>           */
  HTML_TEXT       = 20,  /* <textarea>        */

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
HtmlDoc *htmlScriptNode(const char *scriptText);

HtmlTagType htmlGetTag(const HtmlDoc *doc);
_Bool htmlCanHaveAttr(const HtmlDoc *doc);
_Bool htmlCanHaveSubDoc(const HtmlDoc *doc);
void htmlAddAttr(HtmlDoc *doc, StringPair attrPair);
void htmlAddSubDoc(HtmlDoc *doc, HtmlDoc *subDoc);
void deleteHtml(HtmlDoc *doc);

int printHtml(FILE *fp, HtmlDoc *doc);

#endif /* HTML_H */
