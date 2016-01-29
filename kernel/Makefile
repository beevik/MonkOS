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

FILES       := main.c start.asm strings.asm interrupt.asm io.asm
OBJ_FILES   := $(foreach F,$(FILES),$(F:.c=.o))
OBJ_FILES   := $(foreach F,$(OBJ_FILES),$(F:.asm=.o))
OBJ_FILES   := $(foreach F,$(OBJ_FILES),$(patsubst %,$(DIR_BUILD)/%,$(F)))
TAG         := $(BLUE)[kernel]$(NORMAL)

all: .deps $(DIR_BUILD)/monk64.sys
	@echo "$(TAG) $(SUCCESS)"

clean:
	@rm -f $(OBJ_FILES)

$(DIR_BUILD)/start.o: start.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/main.o: main.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DIR_BUILD)/strings.o: strings.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/interrupt.o: interrupt.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/io.o: io.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DIR_BUILD)/monk64.sys: linker.ld $(OBJ_FILES)
	@echo "$(TAG) Linking $(notdir $@)"
	@$(CC) $(LDFLAGS) -T linker.ld -o $@ $(OBJ_FILES)
