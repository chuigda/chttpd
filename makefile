CCPM_CFLAGS ?= -O2
CFLAGS ?= -O0 -g
WARNINGS := -Wall -Wextra -Wc++-compat -Wno-unused-function
LOG := sh -c 'printf \\t$$0\\t$$1\\n'
INCLUDES := -I include -I include_ext

.PHONY: all run
all: \
	all_deps \
	chttpd_main

.PHONY: all_deps
all_deps: \
	include_ext_dir \
	src_ext_dir \
	out_dir \
	cclib \
	util \
	pl2 \
	config \
	html \
	agno3 \
	http

# All headers
HEADERS = include/agno3.h \
	include/config.h \
	include/dcgi.h \
	include/file_util.h \
	include/error.h \
	include/html.h \
	include/http.h \
	include/http_base.h \
	include/pl2b.h \
	include/static.h \
	include_ext/cc_defs.h \
	include_ext/cc_list.h \
	include_ext/cc_vec.h

# Build CHTTPD main program
.PHONY: chttpd_main chttpd_prompt

chttpd_main: chttpd_prompt chttpd

chttpd_prompt:
	@echo Building chttpd executable

chttpd: all_deps src/main.c
	@$(LOG) CC src/main.c
	@$(CC) src/main.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/main.o
	@$(LOG) BUILD chttpd
	@$(CC) out/main.o \
		${HTTP_OBJECTS} \
		${AGNO3_OBJECTS} \
		${HTML_OBJECTS} \
		${CONFIG_OBJECTS} \
		${PL2_OBJECTS} \
		${UTIL_OBJECTS} \
		${CCLIB_OBJECTS} \
		-o chttpd -lpthread -ldl

# Build HTTP objects
HTTP_OBJECTS := out/http.o out/dcgi.o out/static.o

.PHONY: http http_prompt
http: http_prompt ${HTTP_OBJECTS}

http_prompt:
	@echo Building http library

out/http.o: src/http.c ${HEADERS}
	@$(LOG) CC src/http.c
	@$(CC) src/http.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/http.o

out/dcgi.o: src/dcgi.c ${HEADERS}
	@$(LOG) CC src/dcgi.c
	@$(CC) src/dcgi.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/dcgi.o

out/static.o: src/static.c ${HEADERS}
	@$(LOG) CC src/static.c
	@$(CC) src/static.c $(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -o out/static.o

# Build AgNO3 lang objects
AGNO3_OBJECTS := out/agno3.o

.PHONY: agno3 agno3_prompt
agno3: agno3_prompt ${AGNO3_OBJECTS}

agno3_prompt:
	@echo Building AgNO3 html preprocessor

out/agno3.o: src/agno3.c src/agno3_impl.c ${HEADERS}
	@$(LOG) CC src/agno3.c
	@$(CC) src/agno3.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-I src/ -c -o out/agno3.o

# Build HTML library objects
HTML_OBJECTS := out/html.o

.PHONY: html html_prompt
html: html_prompt out/html.o

html_prompt:
	@echo Building HTML generation and rendering library

out/html.o: src/html.c ${HEADERS}
	@$(LOG) CC src/html.c
	@$(CC) src/html.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/html.o

# Build CFG lang objects
CONFIG_OBJECTS := out/config.o

.PHONY: cfglang cfglang_prompt
config: cfglang_prompt ${CONFIG_OBJECTS}

config_prompt:
	@echo Building chttpd configuration language

out/config.o: src/config.c ${HEADERS}
	@$(LOG) CC src/config.c
	@$(CC) src/config.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -o out/config.o

# Build PL2 objects
PL2_OBJECTS := out/pl2b.o

.PHONY: pl2 pl2_prompt
pl2: pl2_prompt ${PL2_OBJECTS}

pl2_prompt:
	@echo Building PL2 scripting library

out/pl2b.o: pl2/pl2b.c ${HEADERS}
	@$(LOG) CC pl2/pl2b.c
	@$(CC) pl2/pl2b.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/pl2b.o

# Build UTIL objects
UTIL_OBJECTS := out/util.o out/file_util.o out/error.o

.PHONY: util util_prompt
util: util_prompt ${UTIL_OBJECTS}

util_prompt:
	@echo Building UTIL

out/util.o: src/util.c ${HEADERS}
	@$(LOG) CC src/util.c
	@$(CC) src/util.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/util.o

out/file_util.o: src/file_util.c ${HEADERS}
	@$(LOG) CC src/file_util.c
	@$(CC) src/file_util.c $(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -o out/file_util.o

out/error.o: src/error.c ${HEADERS}
	@$(LOG) CC src/error.c
	@$(CC) src/error.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/error.o

# Build CCLIB objects
CCLIB_OBJECTS := out/cc_vec.o out/cc_list.o

.PHONY: cclib cclib_prompt
cclib: prep cclib_prompt ${CCLIB_OBJECTS}

cclib_prompt:
	@echo Building CCLIB

out/cc_list.o: src_ext/cc_list.c ${HEADERS}
	@$(LOG) CC src_ext/cc_list.c
	@$(CC) src_ext/cc_list.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -fPIC -o out/cc_list.o

out/cc_vec.o: src_ext/cc_vec.c ${HEADERS}
	@$(LOG) CC src_ext/cc_vec.c
	@$(CC) src_ext/cc_vec.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -fPIC -o out/cc_vec.o

# Run procedural macro preprocessing
.PHONY: prep prep_prompt
prep: \
	prep_prompt \
	cc_proc_macro \
	include_ext/cc_list.h \
	src_ext/cc_list.c \
	include_ext/cc_vec.h \
	src_ext/cc_vec.c \
	include_ext/cc_defs.h

prep_prompt:
	@echo Running procedural macro preprocessing for CCLIB

include_ext/cc_list.h: cc_proc_macro cclib/cc_list.h
	@$(LOG) PROC include_ext/cc_list.h
	@./cc_proc_macro cclib/cc_list.h include_ext/cc_list.h

src_ext/cc_list.c: cc_proc_macro cclib/cc_list.c
	@$(LOG) PROC src_ext/cc_list.c
	@./cc_proc_macro cclib/cc_list.c src_ext/cc_list.c

include_ext/cc_vec.h: cc_proc_macro cclib/cc_vec.h
	@$(LOG) PROC include_ext/cc_vec.h
	@./cc_proc_macro cclib/cc_vec.h include_ext/cc_vec.h

src_ext/cc_vec.c: cc_proc_macro cclib/cc_vec.c
	@$(LOG) PROC src_ext/cc_vec.c
	@./cc_proc_macro cclib/cc_vec.c src_ext/cc_vec.c

include_ext/cc_defs.h: cc_proc_macro cclib/cc_defs.h
	@$(LOG) PROC include_ext/cc_defs.h
	@./cc_proc_macro cclib/cc_defs.h include_ext/cc_defs.h

.PHONY: out_dir out_dir_prompt
out_dir: out_dir_prompt out/

out_dir_prompt:
	@echo "Creating output directory"

out/:
	@$(LOG) MKDIR out
	@mkdir -p out

# Create auxiliary directories
.PHONY: include_ext_dir include_ext_dir_prompt
include_ext_dir: include_ext_dir_prompt include_ext/

include_ext_dir_prompt:
	@echo "Creating include_ext directory"

include_ext/:
	@$(LOG) MKDIR include_ext
	@mkdir include_ext

.PHONY: src_ext_dir src_ext_dir_prompt
src_ext_dir: src_ext_dir_prompt src_ext/

src_ext_dir_prompt:
	@echo "Creating src_ext directory"

src_ext/:
	@$(LOG) MKDIR src_ext
	@mkdir src_ext

cc_proc_macro: cclib/cc_proc_macro.c
	@$(LOG) BUILD cc_proc_macro
	@$(CC) cclib/cc_proc_macro.c \
		$(WARNINGS) \
		$(CCPM_CFLAGS) \
		-o cc_proc_macro
	@$(LOG) STRIP cc_proc_macro
	@strip cc_proc_macro

# Cleanup
.PHONY: clean
clean:
	rm -rf include_ext
	rm -rf src_ext
	rm -rf out
	rm -f cc_proc_macro
	rm -f chttpd

# Unit testing
.PHONY: test test_prompt
test: \
	test_prompt \
	include_ext_dir \
	src_ext_dir \
	out_dir \
	test_html

test_prompt:
	@echo Running unit-tests

# Test case TEST_HTML
.PHONY: test_html test_html_prompt
test_html: test_html_prompt cclib util html test/test_html.c
	@$(LOG) CC test/test_html.c
	@$(CC) test/test_html.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) \
		-c -o out/test_html.o
	@$(LOG) BUILD out/test_html
	@$(CC) \
		${CCLIB_OBJECTS} \
		${UTIL_OBJECTS} \
		${HTML_OBJECTS} \
		out/test_html.o \
		-o out/test_html
	@$(LOG) RUN out/test_html
	@out/test_html

test_html_prompt:
	@echo Testing HTML library
