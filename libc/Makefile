#----------------------------------------------------------------------------
# MonkOS libc makefile
#
# Makefile for the c library.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk

DIR_TARGET := $(DIR_BUILD)/libc

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

C_FILES 	:= snprintf.c strlen.c vsnprintf.c
H_FILES		:= stdio.h string.h
ASM_FILES	:= mem.asm

CODE_FILES	:= $(C_FILES) $(ASM_FILES)
HDR_FILES	:= $(foreach F,$(H_FILES),$(patsubst %,$(DIR_INCLUDE)/libc/%,$(F)))

OBJ_FILES	:= $(foreach F,$(CODE_FILES),$(F:.c=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(F:.asm=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(patsubst %,$(DIR_TARGET)/%,$(F)))

TAG			:= $(BLUE)[libc]$(NORMAL)

all: deps libc
	@echo "$(TAG) $(SUCCESS)"

deps:
	@mkdir -p $(DIR_TARGET)

libc: $(DIR_TARGET)/libc.a

uncrustify:
	@echo "$(TAG) Uncrustifying code"
	@$(UNCRUSTIFY) \
		-c $(UNCRUSTIFY_CONFIG) \
		-l C \
		--replace \
		--no-backup \
		$(HDR_FILES) $(C_FILES) > /dev/null 2> /dev/null

clean:
	@rm -rf $(DIR_TARGET)

# libc

$(DIR_TARGET)/libc.a: $(OBJ_FILES)
	@echo "$(TAG) Archiving $(notdir $@)"
	@rm -f $@
	@$(AR) cqs $@ $(OBJ_FILES)

# C files

$(DIR_TARGET)/snprintf.o: snprintf.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_TARGET)/strlen.o: strlen.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_TARGET)/vsnprintf.o: vsnprintf.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

# Assembly files

$(DIR_TARGET)/mem.o: mem.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@
