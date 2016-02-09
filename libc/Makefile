#----------------------------------------------------------------------------
# MonkOS libc makefile
#
# Makefile for the c library.
#----------------------------------------------------------------------------

DIR_ROOT	:= ..

include $(DIR_ROOT)/config.mk

DIR_TARGET	:= $(DIR_BUILD)/libc
DIR_DEPS	:= $(DIR_DEPS)/libc

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

ASM_FILES	:= $(wildcard *.asm)
C_FILES		:= $(wildcard *.c)
H_FILES		:= $(wildcard $(DIR_INCLUDE)/libc/*.h)

OBJ_FILES_ASM	:= $(ASM_FILES:%.asm=$(DIR_TARGET)/%_asm.o)
OBJ_FILES_C	:= $(C_FILES:%.c=$(DIR_TARGET)/%.o)
OBJ_FILES	:= $(OBJ_FILES_ASM) $(OBJ_FILES_C)

DEP_FILES_ASM 	:= $(ASM_FILES:%.asm=$(DIR_DEPS)/%_asm.d)
DEP_FILES_C	:= $(C_FILES:%.c=$(DIR_DEPS)/%.d)
DEP_FILES 	:= $(DEP_FILES_ASM) $(DEP_FILES_C)

LIB_FILE	:= $(DIR_TARGET)/libc.a

TAG		:= $(BLUE)[libc]$(NORMAL)

all: .mkdir $(LIB_FILE)
	@echo "$(TAG) $(SUCCESS)"

.mkdir:
	@mkdir -p $(DIR_TARGET) $(DIR_DEPS)

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

$(LIB_FILE): $(DEP_FILES) $(OBJ_FILES)
	@echo "$(TAG) Archiving $(notdir $@)"
	@rm -f $@
	@$(AR) cqs $@ $(OBJ_FILES)

$(OBJ_FILES_ASM): $(DIR_TARGET)/%_asm.o: %.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(OBJ_FILES_C): $(DIR_TARGET)/%.o: %.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DEP_FILES_ASM): $(DIR_DEPS)/%_asm.d: %.asm | .mkdir
	@echo "$(TAG) Generating dependencies for $<"
	@set -e; \
	  rm -f $@; \
	  $(AS) -M $(ASFLAGS) $< -MT $*_asm.o > $@.$$$$; \
	  sed 's,\($*_asm\)\.o[ :]*,$(DIR_TARGET)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$

$(DEP_FILES_C): $(DIR_DEPS)/%.d: %.c | .mkdir
	@echo "$(TAG) Generating dependencies for $<"
	@set -e; \
	  rm -f $@; \
	  $(CC) -MM $(CCFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,$(DIR_TARGET)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$

-include $(DEP_FILES)
