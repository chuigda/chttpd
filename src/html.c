#include "html.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define HTML_DOC_COMMON \
  HtmlTagType tagType;

typedef struct st_html_doc_base {
  HTML_DOC_COMMON
} HtmlDocBase;

typedef struct st_html_doc_impl {
  HTML_DOC_COMMON

  ccVec TP(StringPair) attrs;
  ccVec TP(HtmlDoc*) subDocs;
} HtmlDocImpl;

typedef struct st_html_custom_doc_impl {
  HTML_DOC_COMMON

  char *customTagName;

  ccVec TP(StringPair) attrs;
  ccVec TP(HtmlDoc*) subDocs;
} HtmlCustomDocImpl;

typedef struct st_html_plain_doc_impl {
  HTML_DOC_COMMON

  char* plain;
} HtmlPlainDocImpl;

const char *htmlTagRepr(HtmlTagType tagType) {
  switch (tagType) {
    case HTML_HTML:   return "html";
    case HTML_TITLE:  return "title";
    case HTML_BR:     return "br";
    case HTML_HR:     return "hr";
    case HTML_H1:     return "h1";
    case HTML_H2:     return "h2";
    case HTML_H3:     return "h3";
    case HTML_H4:     return "h4";
    case HTML_H5:     return "h5";
    case HTML_H6:     return "h6";
    case HTML_DIV:    return "div";
    case HTML_PARA:   return "p";
    case HTML_SPAN:   return "span";
    case HTML_BOLD:   return "b";
    case HTML_DELETE: return "del";
    case HTML_FORM:   return "form";
    case HTML_INPUT:  return "input";
    case HTML_TEXT:   return "text";

    case HTML_SCRIPT_URL:
      assert(0 && "should not use repr on script node");
      return NULL;

    case HTML_SCRIPT:
      assert(0 && "should not use repr on script node");
      return NULL;

    case HTML_CUSTOM:
      assert(0 && "should not use repr on custom node");
      return NULL;

    case HTML_PLAIN:
      assert(0 && "plain node has no repr");
      return NULL;

    default:
      assert(0 && "unreachable");
      return NULL;
  }
}

HtmlDoc *htmlRootDoc(void) {
  HtmlDocImpl *ret = (HtmlDocImpl*)malloc(sizeof(HtmlDocImpl));
  ret->tagType = HTML_HTML;

  ccVecInit(&ret->attrs, sizeof(StringPair));
  ccVecInit(&ret->subDocs, sizeof(HtmlDoc*));
  return (HtmlDoc*)ret;
}

HtmlDoc *htmlTag(HtmlTagType tag) {
  assert(tag != HTML_PLAIN);
  assert(tag != HTML_CUSTOM);

  HtmlDocImpl *ret = (HtmlDocImpl*)malloc(sizeof(HtmlDocImpl));
  ret->tagType = tag;

  ccVecInit(&ret->attrs, sizeof(StringPair));
  ccVecInit(&ret->subDocs, sizeof(HtmlDoc*));
  return (HtmlDoc*)ret;
}

HtmlDoc *htmlCustomTag(const char *customTag) {
  HtmlCustomDocImpl *ret =
    (HtmlCustomDocImpl*)malloc(sizeof(HtmlCustomDocImpl));
  ret->tagType = HTML_CUSTOM;
  ret->customTagName = copyString(customTag);

  ccVecInit(&ret->attrs, sizeof(StringPair));
  ccVecInit(&ret->subDocs, sizeof(HtmlDoc*));
  return (HtmlDoc*)ret;
}

HtmlDoc *htmlPlainNode(const char *plainText) {
  HtmlPlainDocImpl *ret =
    (HtmlPlainDocImpl*)malloc(sizeof(HtmlPlainDocImpl));
  ret->tagType = HTML_PLAIN;
  ret->plain = copyString(plainText);
  return (HtmlDoc*)ret;
}

HtmlTagType htmlGetTag(const HtmlDoc *doc) {
  HtmlDocBase *base = (HtmlDocBase*)doc;
  return base->tagType;
}

_Bool htmlCanHaveAttr(const HtmlDoc *doc) {
  HtmlTagType tagType = htmlGetTag(doc);
  switch (tagType) {
    case HTML_SCRIPT_URL:
    case HTML_SCRIPT:
    case HTML_PLAIN:
      return 0;
    default:
      return 1;
  }
}

void addAttribute(HtmlDoc *doc, StringPair attrPair) {
  assert(htmlCanHaveAttr(doc));

  HtmlTagType tagType = htmlGetTag(doc);
  if (tagType == HTML_CUSTOM) {
    HtmlCustomDocImpl *customImpl = (HtmlCustomDocImpl*)doc;
    ccVecPushBack(&customImpl->attrs, &attrPair);
  } else {
    HtmlDocImpl *docImpl = (HtmlDocImpl*)doc;
    ccVecPushBack(&docImpl->attrs, &attrPair);
  }
}

void addSubDoc(HtmlDoc *doc, HtmlDoc *subDoc) {
  assert(htmlCanHaveSubDoc(doc));

  HtmlTagType tagType = htmlGetTag(doc);
  if (tagType == HTML_CUSTOM) {
    HtmlCustomDocImpl *customImpl = (HtmlCustomDocImpl*)doc;
    ccVecPushBack(&customImpl->subDocs, &subDoc);
  } else {
    HtmlDocImpl *docImpl = (HtmlDocImpl*)doc;
    ccVecPushBack(&docImpl->subDocs, &subDoc);
  }
}

void deleteHtml(HtmlDoc *doc) {
  // TODO
  (void)doc;
}
