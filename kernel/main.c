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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <interrupt.h>
#include <keyboard.h>
#include <console.h>

#if defined(__linux__)
#   error "This code must be compiled with a cross-compiler."
#endif

//----------------------------------------------------------------------------
//  @function   hexchar
/// @brief      Convert a 4-byte integer to its hexadecimal representation.
/// @param[in]  value   An integer value between 0 and 15.
/// @returns    A character representing the hexadecimal digit.
//----------------------------------------------------------------------------
static inline char
hexchar(int value)
{
    if (value >= 0 && value <= 9)
        return (char)(value + '0');
    else
        return (char)(value - 10 + 'a');
}

//----------------------------------------------------------------------------
//  @function   kmain
/// @brief      Kernel main entry point.
/// @details    kmain is the first function called by the kernel bootstrapper
///             in start.asm.
//----------------------------------------------------------------------------
void
kmain()
{
    // Initialize the console screen.
    console_init();
    console_set_textcolor(0, TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
    console_clear(0);

    // Initialize all interrupt data structures.
    interrupts_init();

    // Initialize the keyboard.
    kb_init();

    // Turn on interrupt service routines.
    interrupts_enable();

    // Display a welcome message on each virtual console.
    for (int id = 0; id < MAX_CONSOLES; id++) {
        console_print(id, "Welcome to \033[e]MonkOS\033[-] (v0.1).\n");
        console_set_textcolor_fg(id, TEXTCOLOR_LTGRAY);
    }

    for (;;) {
        halt();

        // Output the key code every time there's an interrupt.
        key_t key;
        bool avail;
        while ((avail = kb_getkey(&key)) != false) {
            if (key.ch) {
                char buf[] = "Keycode: \033[ ]  \033[-] meta=   ' '\n";
                buf[11] = key.brk ? 'e' : '2';
                buf[13] = hexchar(key.code >> 4);
                buf[14] = hexchar(key.code & 0xf);
                buf[25] = hexchar(key.meta >> 4);
                buf[26] = hexchar(key.meta & 0xf);
                buf[29] = key.ch;
                console_print(0, buf);
            }
            else {
                char buf[] = "Keycode: \033[ ]  \033[-] meta=  \n";
                buf[11] = key.brk ? 'e' : '2';
                buf[13] = hexchar(key.code >> 4);
                buf[14] = hexchar(key.code & 0xf);
                buf[25] = hexchar(key.meta >> 4);
                buf[26] = hexchar(key.meta & 0xf);
                console_print(0, buf);
            }
        }
    }
}
