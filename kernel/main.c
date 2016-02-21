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

#include <core.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <kernel/x86/cpu.h>
#include <kernel/debug/log.h>
#include <kernel/device/keyboard.h>
#include <kernel/device/timer.h>
#include <kernel/device/tty.h>
#include <kernel/interrupt/exception.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/acpi.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/table.h>
#include <kernel/syscall/syscall.h>

#if defined(__linux__)
#    error "This code must be compiled with a cross-compiler."
#endif

#define TTY_DEFAULT  0
#define TTY_KB       2
#define TTY_LOG      3

static void
sysinit()
{
    // Interrupt initialization
    interrupts_init();
    exceptions_init();

    // Device initialization
    tty_init();
    kb_init();
    timer_init(20);              // 20Hz

    // Memory initialization
    memtable_init();
    page_init();
    acpi_init();

    // System call initialization
    syscall_init();

    // Let the games begin
    enable_interrupts();
}

static void
on_log(loglevel_t level, const char *msg)
{
    (void)level;
    tty_print(TTY_LOG, msg);
    tty_print(TTY_LOG, "\n");
}

static void
log_apic_stats()
{
    const struct acpi_madt *madt = acpi_madt();
    if (madt == NULL) {
        logf(LOG_INFO, "No ACPI MADT detected.");
        return;
    }

    logf(LOG_INFO, "Local APIC addr: %#x", madt->ptr_local_apic);

    const struct acpi_madt_local_apic *local = NULL;
    while ((local = acpi_next_local_apic(local)) != NULL) {
        logf(LOG_INFO, "Local APIC id %u: %s",
             local->apicid,
             (local->flags & 1) ? "Usable" : "Unusable");
    }

    const struct acpi_madt_io_apic *io = NULL;
    while ((io = acpi_next_io_apic(io)) != NULL) {
        logf(LOG_INFO, "I/O APIC id %u: Addr=%#x Base=%u",
             io->apicid,
             io->ptr_io_apic,
             io->interrupt_base);
    }

    const struct acpi_madt_iso *iso = NULL;
    while ((iso = acpi_next_iso(iso)) != NULL) {
        logf(LOG_INFO, "ISO irq=%-2u int=%-2u flags=0x%04x",
             iso->source,
             iso->interrupt,
             iso->flags);
    }
}

static void
do_test()
{
    log_addcallback(LOG_DEFAULT, on_log);
    log_apic_stats();

    // Test code below...
    for (;;) {
        halt();

        // Output the key code every time there's an interrupt.
        key_t key;
        bool  avail;
        while ((avail = kb_getkey(&key)) != false) {
            if (key.ch) {
                tty_printf(
                    TTY_KB,
                    "Keycode: \033[%c]%02x\033[-] meta=%02x '%c'\n",
                    key.brk ? 'e' : '2',
                    key.code,
                    key.meta,
                    key.ch);
            }
            else {
                tty_printf(
                    TTY_KB,
                    "Keycode: \033[%c]%02x\033[-] meta=%02x\n",
                    key.brk ? 'e' : '2',
                    key.code,
                    key.meta);
            }

            if ((key.brk == 0) && (key.meta & META_ALT)) {
                if ((key.code >= '0') && (key.code <= '3')) {
                    int ttyid = key.code - '0';
                    tty_activate(ttyid);
                }
            }
        }
    }
}

void
kmain()
{
    sysinit();

    // Display a welcome message on each virtual console.
    for (int id = 0; id < MAX_TTYS; id++) {
        tty_set_textcolor(id, TEXTCOLOR_LTGRAY, TEXTCOLOR_BLACK);
        tty_clear(id);
        tty_printf(id,
                   "Welcome to \033[e]MonkOS\033[-] (v0.1). [tty=%d]\n", id);
    }

    do_test();
}
