#----------------------------------------------------------------------------
# MonkOS makefile shared configuration
#
# Contains tool, directory, and rule settings shared by all subprojects.
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Project directories
#----------------------------------------------------------------------------
DIR_BUILD   :=  $(DIR_ROOT)/build
DIR_BOOT    :=  $(DIR_ROOT)/boot
DIR_KERNEL  :=  $(DIR_ROOT)/kernel
DIR_DOCS    :=  $(DIR_ROOT)/docs
DIR_SCRIPT  :=  $(DIR_ROOT)/scripts


#----------------------------------------------------------------------------
# Tool configuration
#----------------------------------------------------------------------------
TARGET      :=  x86_64-elf

CC          :=  $(TARGET)-gcc

CCFLAGS     :=  -std=gnu11 -m64 -mno-red-zone -mno-mmx -mno-sse -masm=intel \
                -mno-sse2 -ffreestanding -fno-asynchronous-unwind-tables \
                -I$(DIR_KERNEL)/include -g -Wall -Wextra -Qn

AS          :=  nasm

ASFLAGS     :=  -f elf64

LDFLAGS     :=  -m64 -ffreestanding -g -nostdlib -z max-page-size=0x1000 \
                -lgcc -mno-red-zone

DOXYGEN     :=  doxygen

MAKE_FLAGS  := --quiet

QEMU        :=  qemu-system-x86_64

UNCRUSTIFY  :=  uncrustify

UNCRUSTIFY_CONFIG := $(DIR_SCRIPT)/uncrustify.cfg


#----------------------------------------------------------------------------
# Display color macros
#----------------------------------------------------------------------------
BLUE    := \033[1;34m
YELLOW  := \033[1;33m
NORMAL  := \033[0m

SUCCESS := $(YELLOW)SUCCESS$(NORMAL)


#----------------------------------------------------------------------------
# Shared rules
#----------------------------------------------------------------------------
.deps:
	@mkdir -p $(DIR_BUILD)

.force:
