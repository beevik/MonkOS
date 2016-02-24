//============================================================================
/// @file       main.c
/// @brief      The kernel's main entry point.
/// @details    This file contains the function kmain(), which is the first
///             function called by the kernel's start code in start.asm.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <kernel/device/keyboard.h>
#include <kernel/device/timer.h>
#include <kernel/device/tty.h>
#include <kernel/interrupt/exception.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/acpi.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/table.h>
#include <kernel/syscall/syscall.h>
#include "shell.h"

#if defined(__linux__)
#    error "This code must be compiled with a cross-compiler."
#endif

#define TTY_CONSOLE  0

static void
sysinit()
{
    // Memory initialization
    acpi_init();
    memtable_init();
    page_init();

    // Interrupt initialization
    interrupts_init();
    exceptions_init();

    // Device initialization
    tty_init();
    kb_init();
    timer_init(20);     // 20Hz

    // System call initialization
    syscall_init();

    // Let the games begin
    enable_interrupts();
}

void
kmain()
{
    // Initialize all kernel subsystems.
    sysinit();

    // Display a welcome message on the virtual console.
    tty_set_textcolor(TTY_CONSOLE, TEXTCOLOR_LTGRAY, TEXTCOLOR_BLACK);
    tty_clear(TTY_CONSOLE);
    tty_print(TTY_CONSOLE, "Welcome to \033[e]MonkOS\033[-] (v0.1).\n");

    // Launch the interactive test shell.
    kshell();
}
