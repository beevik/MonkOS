#----------------------------------------------------------------------------
# MonkOS kernel makefile
#
# Makefile for the kernel.
#----------------------------------------------------------------------------

DIR_ROOT	:= ..
LIB_NAME	:= kernel
LIB_DEPS	:= libc
POST_LIB_RULE	:= kernel
LD_FILE		:= kernel.ld

include $(DIR_ROOT)/scripts/lib.mk

kernel: $(DIR_BUILD)/monk.sys

$(DIR_BUILD)/monk.sys: $(LD_FILE) $(LIB_FILE) $(LIB_DEPS_PATHS)
	@echo "$(TAG) Linking $(notdir $@)"
	@$(CC) $(LDFLAGS) -T $(LD_FILE) -o $@ $(LIB_FILE) $(LIB_DEPS_PATHS)
	@chmod a-x $@
