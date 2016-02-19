#----------------------------------------------------------------------------
# lib.mk
# MonkOS makefile rules for building a library
#
# The following variables should be set before including this file:
#
#    DIR_ROOT        The root directory of the project
#    LIB_NAME        The name of the library produced by this makefile
#    LIB_DEPS        The optional list of libraries the lib depends on
#    POST_LIB_RULE   The optional rule that runs after the lib is built
#
#----------------------------------------------------------------------------

include $(DIR_ROOT)/scripts/config.mk


# ---------
# Functions
# ---------

# Recursively find files from the current working directory that match param1.
findfiles	= $(patsubst ./%,%,$(shell find . -name $(1)))

# Recursively find files from the param1 directory that match param2.
findfiles2	= $(shell find $(1) -name $(2))

# Calculate the list of output directories required by one or more files. The
# base output directory is given in param1, and the list of files is given in
# param2.
outputdirs	= $(addprefix $(dir $(1)/), $(sort $(dir $(2))))


# -----------------------
# Configuration variables
# -----------------------

DIR_LIB_BUILD	:= $(DIR_BUILD)/$(LIB_NAME)
DIR_LIB_DEPS	:= $(DIR_DEPS)/$(LIB_NAME)

ASM_FILES	:= $(call findfiles,'*.asm')
C_FILES		:= $(call findfiles,'*.c')
CODE_FILES	:= $(ASM_FILES) $(C_FILES)
H_FILES		:= $(call findfiles,'*.h') \
		   $(call findfiles2,$(DIR_INCLUDE)/$(LIB_NAME),'*.h')

OBJ_FILES_ASM	:= $(ASM_FILES:%.asm=$(DIR_LIB_BUILD)/%_asm.o)
OBJ_FILES_C	:= $(C_FILES:%.c=$(DIR_LIB_BUILD)/%.o)
OBJ_FILES	:= $(OBJ_FILES_ASM) $(OBJ_FILES_C)

DEP_FILES_ASM 	:= $(ASM_FILES:%.asm=$(DIR_LIB_DEPS)/%_asm.d)
DEP_FILES_C	:= $(C_FILES:%.c=$(DIR_LIB_DEPS)/%.d)
DEP_FILES 	:= $(DEP_FILES_ASM) $(DEP_FILES_C)

LIB_FILE	:= $(DIR_LIB_BUILD)/$(LIB_NAME).a

LIB_DEPS_PATHS	:= $(join $(LIB_DEPS:%=$(DIR_BUILD)/%), $(LIB_DEPS:%=/%.a))

TAG		:= $(BLUE)[$(LIB_NAME)]$(NORMAL)


# -----------
# Build rules
# -----------

all: mkdir $(LIB_FILE) $(POST_LIB_RULE)
	@echo "$(TAG) $(SUCCESS)"

mkdir: .force
	@mkdir -p $(call outputdirs,$(DIR_LIB_BUILD),$(CODE_FILES))
	@mkdir -p $(call outputdirs,$(DIR_LIB_DEPS),$(CODE_FILES))

uncrustify:
	@echo "$(TAG) Uncrustifying code"
	@$(UNCRUSTIFY) \
	  -c $(UNCRUSTIFY_CFG) \
	  -l C \
	  --replace \
	  --no-backup \
	  $(C_FILES) $(H_FILES) > /dev/null 2> /dev/null

clean:
	@rm -f $(LIB_FILE) $(OBJ_FILES)

$(LIB_FILE): $(OBJ_FILES) $(DEP_FILES)
	@echo "$(TAG) Archiving $(notdir $@)"
	@rm -f $@
	@$(AR) cqs $@ $(OBJ_FILES)

$(OBJ_FILES_ASM): $(DIR_LIB_BUILD)/%_asm.o: %.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(OBJ_FILES_C): $(DIR_LIB_BUILD)/%.o: %.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DEP_FILES_ASM): $(DIR_LIB_DEPS)/%_asm.d: %.asm | mkdir
	@echo "$(TAG) Generating dependencies for $<"
	@set -e; \
	  rm -f $@; \
	  $(AS) -M $(ASFLAGS) $< -MT @@@ > $@.$$$$; \
	  sed 's,@@@[ :]*,$(DIR_LIB_BUILD)/$*_asm.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$

$(DEP_FILES_C): $(DIR_LIB_DEPS)/%.d: %.c | mkdir
	@echo "$(TAG) Generating dependencies for $<"
	@set -e; \
	  rm -f $@; \
	  $(CC) -MM -MT @@@ $(CCFLAGS) $< > $@.$$$$; \
	  sed 's,@@@[ :]*,$(DIR_LIB_BUILD)/$*.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$

.force:


# ----------------------
# Generated dependencies
# ----------------------

-include $(DEP_FILES)
