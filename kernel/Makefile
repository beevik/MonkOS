#----------------------------------------------------------------------------
# MonkOS kernel makefile
#
# Makefile for the kernel.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

C_FILES 	:= main.c console.c keyboard.c
ASM_FILES	:= interrupt.asm io.asm start.asm strings.asm
CODE_FILES	:= $(C_FILES) $(ASM_FILES)

H_FILES		:= console.h interrupt.h io.h keyboard.h strings.h
H_FILES		:= $(foreach F,$(H_FILES),$(patsubst %,include/%,$(F)))

OBJ_FILES	:= $(foreach F,$(CODE_FILES),$(F:.c=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(F:.asm=.o))
OBJ_FILES	:= $(foreach F,$(OBJ_FILES),$(patsubst %,$(DIR_BUILD)/%,$(F)))
TAG			:= $(BLUE)[kernel]$(NORMAL)

all: .deps kernel
	@echo "$(TAG) $(SUCCESS)"

kernel: $(DIR_BUILD)/monk64.sys

uncrustify:
	@echo "$(TAG) Uncrustifying code"
	@$(UNCRUSTIFY) \
		-c $(UNCRUSTIFY_CONFIG) \
		-l C \
		--replace \
		--no-backup \
		$(H_FILES) $(C_FILES) > /dev/null 2> /dev/null

clean:
	@rm -f $(OBJ_FILES)

$(DIR_BUILD)/main.o: main.c $(H_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_BUILD)/console.o: console.c $(H_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_BUILD)/interrupt.o: interrupt.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/io.o: io.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/keyboard.o: keyboard.c $(H_FILES)
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_BUILD)/start.o: start.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/strings.o: strings.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/monk64.sys: linker.ld $(OBJ_FILES)
	@echo "$(TAG) Linking $(notdir $@)"
	@$(CC) $(LDFLAGS) -T linker.ld -o $@ $(OBJ_FILES)
