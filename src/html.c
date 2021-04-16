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

HtmlDoc *htmlScriptNode(const char *scriptText) {
  HtmlPlainDocImpl *ret =
    (HtmlPlainDocImpl*)malloc(sizeof(HtmlPlainDocImpl));
  ret->tagType = HTML_SCRIPT;
  ret->plain = copyString(scriptText);
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

_Bool htmlCanHaveSubDoc(const HtmlDoc *doc) {
  HtmlTagType tagType = htmlGetTag(doc);
  switch (tagType) {
    case HTML_BR:
    case HTML_HR:
    case HTML_SCRIPT_URL:
    case HTML_SCRIPT:
    case HTML_PLAIN:
      return 0;
    default:
      return 1;
  }
}

void htmlAddAttr(HtmlDoc *doc, StringPair attrPair) {
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

void htmlAddSubDoc(HtmlDoc *doc, HtmlDoc *subDoc) {
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

static int printHtmlAttrs(FILE *fp, ccVec TP(StringPair) *attrs);
static int printHtmlDoc(FILE *fp, 
                        HtmlDocImpl *doc,
                        const char *tagName,
                        _Bool lineBreak);
static int printHtmlLeafDoc(FILE *fp,
                            HtmlDocImpl *doc,
                            const char *tagName);
static int printHtmlCustomDoc(FILE *fp,
                              HtmlCustomDocImpl *doc,
                              _Bool lineBreak);
static _Bool needLineBreak(HtmlTagType tagType);
static int printHtmlImpl(FILE *fp, HtmlDoc *doc, _Bool lineBreak);

#define CHK_NEG_RET(value) \
  {\
    if ((value) < 0) { \
      return (value); \
    } \
  }\

int printHtml(FILE *fp, HtmlDoc *doc) {
  return printHtmlImpl(fp, doc, 1);
}

static _Bool needLineBreak(HtmlTagType tagType) {
  switch (tagType) {
    case HTML_SPAN:
    case HTML_BOLD:
    case HTML_DELETE:
    case HTML_PLAIN:
      return 0;
    default:
      return 1;
  }
}

static int printHtmlImpl(FILE *fp, HtmlDoc *doc, _Bool lineBreak) {
  HtmlDocBase *docBase = (HtmlDocBase*)doc;
  switch (docBase->tagType) {
    case HTML_BR:
      return printHtmlLeafDoc(fp, (HtmlDocImpl*)doc, "br");
    case HTML_HR:
      return printHtmlLeafDoc(fp, (HtmlDocImpl*)doc, "hr");
    case HTML_META:
      return printHtmlLeafDoc(fp, (HtmlDocImpl*)doc, "meta");
    
    case HTML_HTML:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "html", lineBreak);
    case HTML_BODY:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "body", lineBreak);
    case HTML_TITLE:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "title", 0);
    case HTML_H1:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h1", 0);
    case HTML_H2:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h2", 0);
    case HTML_H3:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h3", 0);
    case HTML_H4:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h4", 0);
    case HTML_H5:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h5", 0);
    case HTML_H6:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "h6", 0);

    case HTML_DIV:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "div", lineBreak);
    case HTML_PARA:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "p", lineBreak);

    case HTML_SPAN:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "span", 0);
    case HTML_BOLD:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "b", 0);
    case HTML_DELETE:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "del", 0);

    case HTML_FORM:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "form", lineBreak);
      
    case HTML_INPUT:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "input", lineBreak);

    case HTML_TEXT:
      return printHtmlDoc(fp, (HtmlDocImpl*)doc, "text", lineBreak);

    case HTML_SCRIPT_URL: assert(0 && "unimplemented"); return -1;
    case HTML_SCRIPT: 
      {
        HtmlPlainDocImpl *plain = (HtmlPlainDocImpl*)doc;
        return fprintf(fp, "<script>\n%s\n</script>", plain->plain);
      }

    case HTML_CUSTOM: 
      return printHtmlCustomDoc(fp, (HtmlCustomDocImpl*)doc, lineBreak);

    case HTML_PLAIN:
      {
        HtmlPlainDocImpl *plain = (HtmlPlainDocImpl*)doc;
        return fprintf(fp, "%s", plain->plain);
      }
  }
  
  return -1;
}

static int printHtmlAttrs(FILE *fp, ccVec TP(StringPair) *attrs) {
  int ret = 0;
  
  size_t attrLen = ccVecLen(attrs);
  for (size_t i = 0; i < attrLen; i++) {
    StringPair *attr = (StringPair*)ccVecNth(attrs, i);
    int r1 = fprintf(fp, " %s=\"%s\"", attr->first, attr->second);
    CHK_NEG_RET(r1);
    ret += r1;
  }
  
  return ret;
}

static int printHtmlDoc(FILE *fp,
                        HtmlDocImpl *doc,
                        const char *tagName,
                        _Bool lineBreak) {
  int ret = fprintf(fp, "<%s", tagName);
  CHK_NEG_RET(ret);

  int r1 = printHtmlAttrs(fp, &doc->attrs);
  CHK_NEG_RET(r1);
  ret += r1;

  if (lineBreak) {
    r1 = fprintf(fp, ">\n");
  } else {
    r1 = fprintf(fp, ">");
  }
  CHK_NEG_RET(r1);
  ret += r1;

  size_t subDocLen = ccVecLen(&doc->subDocs);
  for (size_t i = 0; i < subDocLen; i++) {
    HtmlDoc *subDoc = *(HtmlDoc**)(ccVecNth(&doc->subDocs, i));
    r1 = printHtmlImpl(fp, subDoc, lineBreak);
    CHK_NEG_RET(r1);
    ret += r1;
    
    if (i == subDocLen - 1) {
      if (lineBreak) {
        r1 = fputc('\n', fp);
        CHK_NEG_RET(r1);
        ret += r1;
      }
    } else {
      HtmlDoc *nextDoc = *(HtmlDoc**)(ccVecNth(&doc->subDocs, i + 1));
      HtmlDocImpl *subDocImpl = (HtmlDocImpl*)subDoc;
      HtmlDocImpl *nextDocImpl = (HtmlDocImpl*)nextDoc;
      if (lineBreak 
          && (needLineBreak(nextDocImpl->tagType)
              || needLineBreak(subDocImpl->tagType))) {
        r1 = fputc('\n', fp);
        CHK_NEG_RET(r1);
        ret += r1;
      }
    }
  }

  r1 = fprintf(fp, "</%s>", tagName);
  CHK_NEG_RET(r1);
  ret += r1;

  return ret;
}

static int printHtmlLeafDoc(FILE *fp,
                            HtmlDocImpl *doc,
                            const char *tagName) {
  assert(ccVecLen(&doc->subDocs) == 0);

  int ret = fprintf(fp, "<%s", tagName);
  CHK_NEG_RET(ret);
  
  int r1 = printHtmlAttrs(fp, &doc->attrs);
  CHK_NEG_RET(r1);
  ret += r1;
  
  r1 = fprintf(fp, "/>");
  CHK_NEG_RET(r1);
  ret += r1;
  
  return ret;
}

static int printHtmlCustomDoc(FILE *fp,
                              HtmlCustomDocImpl *doc,
                              _Bool lineBreak) {
  int ret = fprintf(fp, "<%s", doc->customTagName);
  CHK_NEG_RET(ret);
  
  int r1 = printHtmlAttrs(fp, &doc->attrs);
  CHK_NEG_RET(r1);
  ret += r1;
  
  if (lineBreak) {
    r1 = fprintf(fp, ">\n");
  } else {
    r1 = fprintf(fp, ">");
  }
  CHK_NEG_RET(r1);
  ret += r1;

  size_t subDocLen = ccVecLen(&doc->subDocs);
  for (size_t i = 0; i < subDocLen; i++) {
    HtmlDoc *subDoc = *(HtmlDoc**)(ccVecNth(&doc->subDocs, i));
    r1 = printHtmlImpl(fp, subDoc, lineBreak);
    CHK_NEG_RET(r1);
    ret += r1;
    
    r1 = fprintf(fp, "\n");
    CHK_NEG_RET(r1);
    ret += r1;
  }

  r1 = fprintf(fp, "</%s>", doc->customTagName);
  CHK_NEG_RET(r1);
  ret += r1;
  
  return ret;
}
