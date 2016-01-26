#----------------------------------------------------------------------------
# MonkOS root makefile
#
# Makefile for all kernel and boot loader targets.
#----------------------------------------------------------------------------

DIR_ROOT := .

include $(DIR_ROOT)/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

default: boot kernel iso

boot: .deps
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_BOOT)

kernel: .deps
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_KERNEL)

iso: .force boot kernel
	@echo "$(BLUE)[iso]$(NORMAL) Running mkcdrom.sh"
	@$(DIR_SCRIPT)/mkcdrom.sh 2> /dev/null > /dev/null
	@echo "$(BLUE)[iso] $(SUCCESS)"

docs: .force
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_DOCS)

all: kernel boot iso docs

debug: iso
	@$(QEMU) -gdb tcp::8864 -cdrom $(DIR_BUILD)/monk.iso

debugwait: iso
	@$(QEMU) -S -gdb tcp::8832 -cdrom $(DIR_BUILD)/monk.iso

test: iso
	@$(QEMU) -cdrom $(DIR_BUILD)/monk.iso

clean: .force
	@rm -rf $(DIR_BUILD)
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_DOCS) clean
	@echo "$(BLUE)[clean]$(NORMAL) Generated files deleted"
