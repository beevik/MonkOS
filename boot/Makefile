#----------------------------------------------------------------------------
# MonkOS boot loader makefile
#
# Makefile for the boot loader.
#----------------------------------------------------------------------------

DIR_ROOT	:= ..

include $(DIR_ROOT)/scripts/config.mk

DIR_TARGET	:= $(DIR_BUILD)/boot
DIR_DEPS 	:= $(DIR_DEPS)/boot
ASFLAGS 	:= -f bin

ASM_FILES	:= $(wildcard *.asm)
SYS_FILES	:= $(ASM_FILES:%.asm=$(DIR_BUILD)/%.sys)
DEP_FILES 	:= $(ASM_FILES:%.asm=$(DIR_DEPS)/%.d)

TAG 		:= $(BLUE)[boot]$(NORMAL)

all: mkdir $(SYS_FILES)
	@echo "$(TAG) $(SUCCESS)"

mkdir:
	@mkdir -p $(DIR_TARGET) $(DIR_DEPS)

clean:
	@rm -f $(SYS_FILES)

$(SYS_FILES): $(DIR_BUILD)/%.sys: %.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(DEP_FILES): $(DIR_DEPS)/%.d: %.asm | mkdir
	@echo "$(TAG) Generating dependencies for $<"
	@set -e; \
	  rm -f $@; \
	  $(AS) -M $(ASFLAGS) $< -MT @@@ > $@.$$$$; \
	  sed 's,@@@[ :]*,$(DIR_BUILD)/$*.sys $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$

.force:

-include $(DEP_FILES)
