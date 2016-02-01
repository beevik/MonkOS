#----------------------------------------------------------------------------
# MonkOS kernel makefile
#
# Makefile for the kernel.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk

DIR_TARGET := $(DIR_BUILD)/kernel

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

C_FILES 	:= main.c console.c keyboard.c
H_FILES		:= console.h interrupt.h io.h keyboard.h
ASM_FILES	:= interrupt.asm io.asm start.asm
LIB_FILES 	:= libc/libc.a kernel/libkernel.a

CODE_FILES	:= $(C_FILES) $(ASM_FILES)
HDR_FILES	:= $(foreach F,$(H_FILES),$(patsubst %,$(DIR_INCLUDE)/kernel/%,$(F)))
LIB_FILES 	:= $(foreach F,$(LIB_FILES),$(patsubst %,$(DIR_BUILD)/%,$(F)))

OBJ_FILES	:= $(foreach F,$(CODE_FILES),$(F:.c=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(F:.asm=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(patsubst %,$(DIR_TARGET)/%,$(F)))

TAG			:= $(BLUE)[kernel]$(NORMAL)

all: deps kernel
	@echo "$(TAG) $(SUCCESS)"

deps:
	@mkdir -p $(DIR_TARGET)

kernel: $(DIR_BUILD)/monk.sys

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
	@rm -f $(DIR_BUILD)/monk.sys

# kernel file
$(DIR_BUILD)/monk.sys: linker.ld $(DIR_TARGET)/libkernel.a
	@echo "$(TAG) Linking $(notdir $@)"
	@$(CC) $(LDFLAGS) -T linker.ld -o $@ $(OBJ_FILES) $(LIB_FILES)
	@chmod a-x $@

# Library file
$(DIR_TARGET)/libkernel.a: $(OBJ_FILES)
	@echo "$(TAG) Archiving $(notdir $@)"
	@rm -f $@
	@$(AR) cqs $@ $(OBJ_FILES)

# Assembly files

$(DIR_TARGET)/interrupt.o: interrupt.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_TARGET)/io.o: io.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_TARGET)/start.o: start.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

# C files

$(DIR_TARGET)/main.o: main.c $(HDR_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_TARGET)/console.o: console.c $(HDR_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_TARGET)/keyboard.o: keyboard.c $(HDR_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@
