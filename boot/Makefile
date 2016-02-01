#----------------------------------------------------------------------------
# MonkOS boot loader makefile
#
# Makefile for the boot loader.
#----------------------------------------------------------------------------

DIR_ROOT := ..

include $(DIR_ROOT)/config.mk

DIR_TARGET := $(DIR_BUILD)/boot

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

FILES 		:= boot.asm loader.asm

SYS_FILES 	:= $(foreach F,$(FILES),$(patsubst %.asm,$(DIR_BUILD)/%.sys,$(F)))

TAG 		:= $(BLUE)[boot]$(NORMAL)

all: deps $(SYS_FILES)
	@echo "$(TAG) $(SUCCESS)"

deps:
	@mkdir -p $(DIR_TARGET)

clean:
	@rm -rf $(DIR_TARGET) $(SYS_FILES)

$(DIR_BUILD)/boot.sys: boot.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) -f bin $< -o $@

$(DIR_BUILD)/loader.sys: loader.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) -f bin $< -o $@
