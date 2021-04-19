#include "html.h"
#include "vktest.h"

static void test_html_print(void) {
  static const char *TEST_NAME = "TestHtmlPrint";

  VK_TEST_SECTION_BEGIN(TEST_NAME)

  HtmlDoc *rootDoc = htmlRootDoc();

  HtmlDoc *metaDoc = htmlTag(HTML_META);
  htmlAddAttr(metaDoc, makeStringPair("charset", "utf-8"));
  htmlAddSubDoc(rootDoc, metaDoc);

  HtmlDoc *titleDoc = htmlTag(HTML_TITLE);
  htmlAddSubDoc(titleDoc, htmlPlainNode("Privet"));
  htmlAddSubDoc(rootDoc, titleDoc);

  HtmlDoc *headDoc = htmlTag(HTML_HEAD);
  htmlAddSubDoc(
    headDoc,
    htmlScriptUrlNode(
      "https://cdn.jsdelivr.net/npm/vue/dist/vue.js"
    )
  );
  htmlAddSubDoc(rootDoc, headDoc);

  HtmlDoc *bodyDoc = htmlTag(HTML_BODY);

  htmlAddSubDoc(bodyDoc, htmlPlainNode("Privet, "));
  HtmlDoc *span1 = htmlTag(HTML_BOLD);
  htmlAddSubDoc(span1, htmlPlainNode("tovarisc"));
  htmlAddSubDoc(bodyDoc, span1);
  htmlAddSubDoc(bodyDoc, htmlPlainNode("!"));

  HtmlDoc *div1 = htmlTag(HTML_DIV);
  htmlAddAttr(div1, makeStringPair("id", "vue"));
  htmlAddSubDoc(bodyDoc, div1);

  HtmlDoc *button = htmlCustomTag("button");
  htmlAddSubDoc(button, htmlPlainNode(
    "click me to {{ show ? 'hide' : 'show' }}"
  ));
  htmlAddAttr(button, makeStringPair("v-on:click", "alterVisibility"));
  htmlAddSubDoc(div1, button);

  HtmlDoc *span2 = htmlTag(HTML_SPAN);
  htmlAddAttr(span2, makeStringPair("v-if", "show"));
  htmlAddSubDoc(span2, htmlPlainNode("hidden message"));
  htmlAddSubDoc(div1, span2);

  htmlAddSubDoc(bodyDoc, htmlScriptNode(
    "var app = new Vue({\n"
    "  el: '#vue',\n"
    "  data: {\n"
    "    show: false,\n"
    "  },\n"
    "  methods: {\n"
    "    alterVisibility: function() {\n"
    "      this.show = !this.show\n"
    "    }\n"
    "  }\n"
    "})"
  ));

  htmlAddSubDoc(rootDoc, bodyDoc);

  printHtml(stderr, rootDoc);
  fputc('\n', stderr);

  VK_TEST_SECTION_END(TEST_NAME)

  deleteHtml(rootDoc);
}

int main() {
  VK_TEST_BEGIN

  test_html_print();

  VK_TEST_END
}
