#----------------------------------------------------------------------------
# MonkOS root makefile
#
# Makefile for all kernel and boot loader targets.
#----------------------------------------------------------------------------

DIR_ROOT := .

include $(DIR_ROOT)/scripts/config.mk


#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

default: boot kernel iso

all: boot kernel iso tags docs

boot: .force
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_BOOT)

kernel: .force libc
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_KERNEL)

libc: .force
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_LIBC)

iso: .force boot kernel
	@echo "$(BLUE)[iso]$(NORMAL) Running mkcdrom.sh"
	@$(DIR_SCRIPTS)/mkcdrom.sh 2> /dev/null > /dev/null
	@echo "$(BLUE)[iso] $(SUCCESS)"

docs: .force
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_DOCS)

tags: .force
	@echo "$(BLUE)[tags]$(NORMAL) Running exuberant-ctags"
	@$(CTAGS) -R -f .tags
	@echo "$(BLUE)[tags] $(SUCCESS)"

uncrustify: .force
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_LIBC) uncrustify
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_KERNEL) uncrustify

debug: iso
	@$(QEMU) -gdb tcp::8864 -cdrom $(DIR_BUILD)/monk.iso

debugwait: iso
	@$(QEMU) -S -gdb tcp::8864 -cdrom $(DIR_BUILD)/monk.iso

test: iso
	@$(QEMU) -cdrom $(DIR_BUILD)/monk.iso

clean: .force
	@rm -rf $(DIR_BUILD)
	@$(MAKE) $(MAKE_FLAGS) --directory=$(DIR_DOCS) clean
	@echo "$(BLUE)[clean]$(NORMAL) Generated files deleted"

cleandeps: .force
	@rm -rf $(DIR_DEPS)
	@echo "$(BLUE)[clean]$(NORMAL) Dependency files deleted"

.force:
