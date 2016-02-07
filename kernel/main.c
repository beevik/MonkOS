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
#include <kernel/console.h>
#include <kernel/exception.h>
#include <kernel/interrupt.h>
#include <kernel/keyboard.h>
#include <kernel/syscall.h>
#include <kernel/timer.h>

#if defined(__linux__)
#   error "This code must be compiled with a cross-compiler."
#endif

//----------------------------------------------------------------------------
static inline char
hexchar(int value)
{
    if ((value >= 0) && (value <= 9))
        return (char)(value + '0');
    else
        return (char)(value - 10 + 'a');
}

//----------------------------------------------------------------------------
static void
sysinit()
{
    // Initialize the interrupt controllers and tables.
    interrupts_init();

    // Initialize the console.
    console_init();
    console_set_textcolor(0, TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
    console_clear(0);

    // Initialize various system modules.
    exceptions_init();
    syscall_init();
    kb_init();
    timer_init(20);         // 20Hz

    // Enable interrupts.
    interrupts_enable();
}

//----------------------------------------------------------------------------
static void
do_test()
{
    // Test code below...
    int console_id = 0;
    for (;;) {
        halt();

        // Output the key code every time there's an interrupt.
        key_t key;
        bool  avail;
        while ((avail = kb_getkey(&key)) != false) {
            if (key.ch) {
                char buf[] = "Keycode: \033[ ]  \033[-] meta=   ' '\n";
                buf[11] = key.brk ? 'e' : '2';
                buf[13] = hexchar(key.code >> 4);
                buf[14] = hexchar(key.code & 0xf);
                buf[25] = hexchar(key.meta >> 4);
                buf[26] = hexchar(key.meta & 0xf);
                buf[29] = key.ch;
                console_print(console_id, buf);
                if (key.ch == 'x')
                    raise(EXCEPTION_BREAKPOINT);
            }
            else {
                char buf[] = "Keycode: \033[ ]  \033[-] meta=  \n";
                buf[11] = key.brk ? 'e' : '2';
                buf[13] = hexchar(key.code >> 4);
                buf[14] = hexchar(key.code & 0xf);
                buf[25] = hexchar(key.meta >> 4);
                buf[26] = hexchar(key.meta & 0xf);
                console_print(console_id, buf);
            }

            if ((key.brk == 0) && (key.meta & META_ALT)) {
                if ((key.code >= '1') && (key.code <= '4')) {
                    console_id = key.code - '1';
                    console_activate(console_id);
                    console_print(console_id, "Console activated.\n");
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
void
kmain()
{
    sysinit();

    // Display a welcome message on each virtual console.
    for (int id = 0; id < MAX_CONSOLES; id++) {
        console_print(id, "Welcome to \033[e]MonkOS\033[-] (v0.1).\n");
        console_set_textcolor_fg(id, TEXTCOLOR_LTGRAY);
    }

    do_test();
}
