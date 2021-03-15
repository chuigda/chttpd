WARNINGS := -Wall -Wextra -Wc++-compat
LOG := sh -c 'printf \\t$$0\\t$$1\\n'
INCLUDES := -I include -I include_ext

.PHONY: all
all: include_ext_dir src_ext_dir out_dir pl2 cclib cfglang http

.PHONY: http http_prompt
http: http_prompt out/libhttp.a

http_prompt:
	@echo Building http library

out/libhttp.a: out/http.o
	@$(LOG) AR out/http.o
	@ar -rcs out/libhttp.a out/http.o

out/http.o: src/http.c include/http.h include/http_base.h
	@$(LOG) CC src/http.c
	@$(CC) src/http.c $(INCLUDES) $(WARNINGS) $(FLAGS) -c -o out/http.o

.PHONY: cfglang cfglang_prompt
cfglang: cfglang_prompt out/libcfg.a

cfglang_prompt:
	@echo Building chttpd configuration language

out/libcfg.a: out/cfglang.o out/libpl2.a
	@$(LOG) AR out/cfglang.o
	@ar -rcs out/libcfg.a out/cfglang.o out/libpl2.a

out/cfglang.o: src/cfglang.c include/cfglang.h include/http_base.h
	@$(LOG) CC src/cfglang.c
	@$(CC) src/cfglang.c $(INCLUDES) $(WARNINGS) $(FLAGS) -c -o out/cfglang.o

.PHONY: pl2 pl2_prompt
pl2: pl2_prompt out/libpl2.a

pl2_prompt:
	@echo Building PL2 scripting library

out/libpl2.a: out/pl2b.o
	@$(LOG) AR out/libpl2.a
	@ar -rcs out/libpl2.a out/pl2b.o
	@$(LOG) CHMOD out/libpl2.a
	@chmod a+x out/libpl2.a

out/pl2b.o: pl2/pl2b.c include/pl2b.h
	@$(LOG) CC pl2/pl2b.c
	@$(CC) pl2/pl2b.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/pl2b.o

.PHONY: cclib cclib_prompt
cclib: prep cclib_prompt out/libcclib.a

cclib_prompt:
	@echo Building CCLIB

out/libcclib.a: out/cc_list.o out/cc_vec.o
	@$(LOG) AR out/libcclib.a
	@ar -rcs out/libcclib.a out/cc_list.o out/cc_vec.o
	@$(LOG) CHMOD out/libcclib.a
	@chmod a+x out/libcclib.a

out/cc_list.o: src_ext/cc_list.c include_ext/cc_list.h include_ext/cc_defs.h
	@$(LOG) CC src_ext/cc_list.c
	@$(CC) src_ext/cc_list.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/cc_list.o

out/cc_vec.o: src_ext/cc_vec.c include_ext/cc_vec.h include_ext/cc_defs.h
	@$(LOG) CC src_ext/cc_vec.c
	@$(CC) src_ext/cc_vec.c $(INCLUDES) $(WARNINGS) $(CFLAGS) -c -o out/cc_vec.o

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

.PHONY: clean
clean:
	rm -rf include_ext
	rm -rf src_ext
	rm -rf out
	rm -f cc_proc_macro
