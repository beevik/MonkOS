#----------------------------------------------------------------------------
# MonkOS boot loader makefile
#
# Makefile for the boot loader.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

FILES 		:= boot_iso.asm loader_iso.asm
OBJ_FILES 	:= $(foreach F,$(FILES),$(patsubst %.asm,$(DIR_BUILD)/%.sys,$(F)))
TAG 		:= $(BLUE)[boot]$(NORMAL)

all: .deps $(OBJ_FILES)
	@echo "$(TAG) $(SUCCESS)"

clean:
	@rm -f $(OBJ_FILES)

$(DIR_BUILD)/boot_iso.sys: boot_iso.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) -f bin boot_iso.asm -o $(DIR_BUILD)/boot_iso.sys

$(DIR_BUILD)/loader_iso.sys: loader_iso.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) -f bin loader_iso.asm -o $(DIR_BUILD)/loader_iso.sys
