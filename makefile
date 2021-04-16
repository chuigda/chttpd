CFLAGS ?= -O0 -g
WARNINGS := -Wall -Wextra -Wc++-compat -Wno-unused-function
LOG := sh -c 'printf \\t$$0\\t$$1\\n'
INCLUDES := -I include -I include_ext

.PHONY: all
all: \
	include_ext_dir \
	src_ext_dir \
	out_dir \
	cclib \
	util \
	pl2 \
	cfglang \
	agno3 \
	http

# All headers
HEADERS = include/agno3.h \
	include/cfglang.h \
	include/html.h \
	include/http.h \
	include/http_base.h \
	include/pl2b.h \
	include_ext/cc_defs.h \
	include_ext/cc_list.h \
	include_ext/cc_vec.h

# Build HTTP objects
HTTP_OBJECTS := out/http.o

.PHONY: http http_prompt
http: http_prompt ${HTTP_OBJECTS}

http_prompt:
	@echo Building http library

out/http.o: src/http.c ${HEADERS}
	@$(LOG) CC src/http.c
	@$(CC) src/http.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/http.o

# Build AgNO3 lang objects
AGNO3_OBJECTS := out/agno3.o out/html.o

.PHONY: agno3 agno3_prompt
agno3: agno3_prompt ${AGNO3_OBJECTS}

agno3_prompt:
	@echo Building AgNO3 html preprocessor

out/agno3.o: src/agno3.c ${HEADERS}
	@$(LOG) CC src/agno3.c
	@$(CC) src/agno3.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/agno3.o

out/html.o: src/html.c ${HEADERS}
	@$(LOG) CC src/html.c
	@$(CC) src/html.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/html.o

# Build CFG lang objects
CFG_LANG_OBJECTS := out/cfglang.o

.PHONY: cfglang cfglang_prompt
cfglang: cfglang_prompt ${CFG_LANG_OBJECTS}

cfglang_prompt:
	@echo Building chttpd configuration language

out/cfglang.o: src/cfglang.c ${HEADERS}
	@$(LOG) CC src/cfglang.c
	@$(CC) src/cfglang.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/cfglang.o

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
UTIL_OBJECTS := out/util.o

.PHONY: util util_prompt
util: util_prompt ${UTIL_OBJECTS}

util_prompt:
	@echo Building UTIL

out/util.o: src/util.c ${HEADERS}
	@$(LOG) CC src/util.c
	@$(CC) src/util.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/util.o

# Build CCLIB objects
CCLIB_OBJECTS := out/cc_vec.o out/cc_list.o

.PHONY: cclib cclib_prompt
cclib: prep cclib_prompt ${CCLIB_OBJECTS}

cclib_prompt:
	@echo Building CCLIB

out/cc_list.o: src_ext/cc_list.c ${HEADERS}
	@$(LOG) CC src_ext/cc_list.c
	@$(CC) src_ext/cc_list.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) -c -fPIC -o out/cc_list.o

out/cc_vec.o: src_ext/cc_vec.c ${HEADERS}
	@$(LOG) CC src_ext/cc_vec.c
	@$(CC) src_ext/cc_vec.c \
		$(INCLUDES) $(WARNINGS) $(CFLAGS) -c -fPIC -o out/cc_vec.o

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
	@$(CC) cclib/cc_proc_macro.c $(WARNINGS) $(CFLAGS) -o cc_proc_macro

# Cleanup
.PHONY: clean
clean:
	rm -rf include_ext
	rm -rf src_ext
	rm -rf out
	rm -f cc_proc_macro

# Unit testing
.PHONY: test test_prompt
test: \
	all \
	test_prompt \
	test_agno3

test_prompt:
	@echo Running unit-tests

# Test case TEST_AGNO3
.PHONY: test_agno3 test_agno3_prompt
test_agno3: \
		test_agno3_prompt \
		cclib util pl2 agno3 \
		test/test_agno3.c	
	@$(LOG) CC test/test_agno3.c
	@$(CC) test/test_agno3.c \
		$(INCLUDES) \
		$(WARNINGS) \
		$(CFLAGS) \
		-c -o out/test_agno3.o
	@$(LOG) BUILD out/test_agno3
	@$(CC) \
		${CCLIB_OBJECTS} \
		${UTIL_OBJECTS} \
		${PL2_OBJECTS} \
		${AGNO3_OBJECTS} \
		out/test_agno3.o \
		-o out/test_agno3
	@$(LOG) RUN out/test_agno3
	@out/test_agno3

test_agno3_prompt:
	@echo Testing AgNO3 library
