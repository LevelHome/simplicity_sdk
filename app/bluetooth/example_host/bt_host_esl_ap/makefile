# Build library dependencies and Python wrappers for the ESL AP script

.PHONY: all lib debug release wrapper clean clean_sql

all: release
debug: ARGS += debug
debug: lib wrapper
release: ARGS += release
release: lib wrapper

lib:
	@$(MAKE) -f ../../common_host/esl_lib/makefile -j $(ARGS)
	@$(MAKE) -f ../../common_host/esl_key_lib/makefile OBJ_DIR=obj_key -j $(ARGS)

wrapper:
	@$(MAKE) -f ../../common_host/esl_lib/makefile esl_lib_wrapper.py
	@$(MAKE) -f ../../common_host/esl_key_lib/makefile esl_key_lib_wrapper.py

clean:
	@$(MAKE) -f ../../common_host/esl_lib/makefile clean
	@$(MAKE) -f ../../common_host/esl_key_lib/makefile OBJ_DIR=obj_key clean_keep_sql

clean_sql:
	@$(MAKE) -f ../../common_host/esl_lib/makefile clean
	@$(MAKE) -f ../../common_host/esl_key_lib/makefile OBJ_DIR=obj_key clean
