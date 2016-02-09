#----------------------------------------------------------------------------
# MonkOS makefile shared configuration
#
# Contains tool, directory, and rule settings shared by all subprojects.
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Project directories
#----------------------------------------------------------------------------
DIR_BOOT	:= $(DIR_ROOT)/boot
DIR_BUILD	:= $(DIR_ROOT)/build
DIR_DEPS	:= $(DIR_ROOT)/deps
DIR_DOCS	:= $(DIR_ROOT)/docs
DIR_INCLUDE	:= $(DIR_ROOT)/include
DIR_KERNEL	:= $(DIR_ROOT)/kernel
DIR_LIBC	:= $(DIR_ROOT)/libc
DIR_SCRIPTS	:= $(DIR_ROOT)/scripts


#----------------------------------------------------------------------------
# Tool configuration
#----------------------------------------------------------------------------
TARGET		:= x86_64-elf

CC		:= $(TARGET)-gcc

CCFLAGS		:= -Og -std=gnu11 -m64 -mno-red-zone -mno-mmx -mfpmath=sse \
		   -ffreestanding -fno-asynchronous-unwind-tables \
		   -masm=intel -I$(DIR_INCLUDE) -g -Wall -Wextra -Qn

AS		:= nasm

ASFLAGS		:= -f elf64

AR		:= $(TARGET)-ar

LDFLAGS		:= -m64 -ffreestanding -g -nostdlib -z max-page-size=0x1000 \
		   -lgcc -mno-red-zone

DOXYGEN		:= doxygen

MAKE_FLAGS	:= --quiet --no-print-directory

QEMU		:= qemu-system-x86_64

UNCRUSTIFY	:= uncrustify

UNCRUSTIFY_CFG	:= $(DIR_SCRIPTS)/uncrustify.cfg


#----------------------------------------------------------------------------
# Display color macros
#----------------------------------------------------------------------------
BLUE		:= \033[1;34m
YELLOW		:= \033[1;33m
NORMAL		:= \033[0m

SUCCESS		:= $(YELLOW)SUCCESS$(NORMAL)


#----------------------------------------------------------------------------
# Shared rules
#----------------------------------------------------------------------------
.force:
